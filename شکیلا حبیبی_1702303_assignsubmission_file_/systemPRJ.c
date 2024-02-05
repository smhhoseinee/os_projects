#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/wait.h>

#define MAX_FILES 2000
#define MAX_PATH 1024
#define MAX_FILE_TYPES 100

struct FileInfo {
    char name[MAX_PATH];
    off_t size;
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
struct FileInfo *allFiles;
int *fileCount;
char *fileTypes[MAX_FILE_TYPES];
int fileTypeCounts[MAX_FILE_TYPES] = {0};

// Define a structure for the message
struct MessageType {
    long messageType;  // Message type
    int typeIndex;     // Index of the file type
    int count;         // Count of the file type
};

char *getFileType(char *path);
int addFileType(char *fileType);
int processFileTypeCounts[MAX_FILE_TYPES] = {0};

void traverseDirectory(char *path, int msgqid, int shmid, int processIndex, pthread_mutex_t *mutex);
void processFileOrDir(char *path, int msgqid, int shmid, int processIndex, pthread_mutex_t *mutex);
void calculateFileCountAndSize(struct FileInfo *files, int numFiles);

int main() {
    char rootDirectory[MAX_PATH];

    // Read the root directory path from the user
    printf("Enter the root directory path: ");
    scanf("%s", rootDirectory);

    // Shared memory initialization
    int shmid = shmget(IPC_PRIVATE, (sizeof(struct FileInfo) * MAX_FILES) + sizeof(int) + (sizeof(char *) * MAX_FILE_TYPES * MAX_FILES), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    allFiles = (struct FileInfo *)shmat(shmid, NULL, 0);
    if (allFiles == (struct FileInfo *)-1) {
        perror("shmat");
        exit(1);
    }

    fileCount = (int *)((void *)allFiles + (sizeof(struct FileInfo) * MAX_FILES));

    // Initialize file count
    *fileCount = 0;

    // Create arrays for file types and counts for each process
    char **processFileTypes = (char **)((void *)fileCount + sizeof(int));
    int *processFileTypeCounts = (int *)((void *)processFileTypes + (sizeof(char *) * MAX_FILE_TYPES * MAX_FILES));

    // Initialize process-specific file type counts
    for (int i = 0; i < MAX_FILE_TYPES; i++) {
        processFileTypes[i] = NULL;
        processFileTypeCounts[i] = 0;
    }

    // Create a message queue
    int msgqid = msgget(IPC_PRIVATE, IPC_CREAT | 0666);
    if (msgqid == -1) {
        perror("msgget");
        exit(1);
    }

    // Create a process for the root directory
    traverseDirectory(rootDirectory, msgqid, shmid, 0, &mutex);

    // Wait for all child processes to finish
    for (int i = 0; i < *fileCount; i++) {
        wait(NULL);
    }

    calculateFileCountAndSize(allFiles, *fileCount);
    // Remove the message queue
    if (msgctl(msgqid, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        exit(1);
    }

    // Detach and remove shared memory
    shmdt(allFiles);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}

char *getFileType(char *path) {
    char *dot = strrchr(path, '.');
    if (!dot || dot == path) {
        return "";  // Hidden file or no extension
    }
    return dot + 1;
}

int addFileType(char *fileType) {
    if (fileType[0] == '\0') {
        return -1;  // Ignore hidden files or files with no extension
    }

    for (int i = 0; i < MAX_FILE_TYPES; i++) {
        if (fileTypes[i] == NULL) {
            fileTypes[i] = strdup(fileType);
            return i;
        } else if (strcmp(fileTypes[i], fileType) == 0) {
            return i;
        }
    }
    return -1;  // No space left for more file types
}

void traverseDirectory(char *path, int msgqid, int shmid, int processIndex, pthread_mutex_t *mutex) {
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(path)) == NULL) {
        perror("Error opening directory");
        fprintf(stderr, "Directory path: %s\n", path);
        return;
    }

    while ((entry = readdir(dir)) != NULL && *fileCount < MAX_FILES) {
        char filePath[MAX_PATH];
        snprintf(filePath, sizeof(filePath), "%s/%s", path, entry->d_name);

        struct stat fileStat;
        if (stat(filePath, &fileStat) == 0) {
            if (S_ISDIR(fileStat.st_mode) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                if (!S_ISLNK(fileStat.st_mode)) {
                    // Create a process for each subdirectory
                    pid_t childPid = fork();
                    if (childPid == -1) {
                        perror("fork");
                        exit(1);
                    } else if (childPid == 0) {
                        // In child process

                        // Call the recursive function for the subdirectory
                        traverseDirectory(filePath, msgqid, shmid, processIndex + 1, mutex);
                        exit(0);
                    } else {
                        // In parent process
                        wait(NULL);
                    }
                }
            } else if (S_ISREG(fileStat.st_mode)) {
                // Call the process function for each regular file
                processFileOrDir(filePath, msgqid, shmid, processIndex, mutex);
            }
        } else {
            perror("Error getting file info");
            fprintf(stderr, "File path: %s\n", filePath);
        }
    }

    closedir(dir);
}

void processFileOrDir(char *path, int msgqid, int shmid, int processIndex, pthread_mutex_t *mutex) {
    struct stat fileInfo;
    if (stat(path, &fileInfo) == 0) {
        pthread_mutex_lock(mutex);

        // Store file information in the array
        strncpy(allFiles[*fileCount].name, path, sizeof(allFiles[*fileCount].name) - 1);
        allFiles[*fileCount].name[sizeof(allFiles[*fileCount].name) - 1] = '\0';
        allFiles[*fileCount].size = fileInfo.st_size;
        (*fileCount)++;

        pthread_mutex_unlock(mutex);

        // Update file type counts
        char *fileType = getFileType(path);
        int typeIndex = addFileType(fileType);
        if (typeIndex != -1) {
            pthread_mutex_lock(mutex);

            // Use processIndex for indexing processFileTypeCounts
        //    fileTypeCounts[typeIndex]++;
            processFileTypeCounts[processIndex]++;

            pthread_mutex_unlock(mutex);

            // Send the count of each file type to the main process
            struct MessageType message;
            message.messageType = 1;  // Any positive value
            message.typeIndex = typeIndex;
            message.count = processFileTypeCounts[processIndex];

            if (msgsnd(msgqid, &message, sizeof(struct MessageType) - sizeof(long), 0) == -1) {
                perror("msgsnd");
                exit(1);
            }
        }
    }
}


void calculateFileCountAndSize(struct FileInfo *files, int numFiles) {
    int totalFiles = 0;
    off_t totalSize = 0;
    char maxFileSizeName[MAX_PATH];
    char minFileSizeName[MAX_PATH];
    off_t maxFileSize = 0;
    off_t minFileSize = -1;

    for (int i = 0; i < numFiles; i++) {
        totalFiles++;
        totalSize += files[i].size;

        if (files[i].size > maxFileSize) {
            maxFileSize = files[i].size;
            strncpy(maxFileSizeName, files[i].name, sizeof(maxFileSizeName) - 1);
            maxFileSizeName[sizeof(maxFileSizeName) - 1] = '\0';
        }

        if (files[i].size < minFileSize || minFileSize == -1) {
            minFileSize = files[i].size;
            strncpy(minFileSizeName, files[i].name, sizeof(minFileSizeName) - 1);
            minFileSizeName[sizeof(minFileSizeName) - 1] = '\0';
        }

        // Update file type counts
        char *fileType = getFileType(files[i].name);
        int typeIndex = addFileType(fileType);
        if (typeIndex != -1) {
            fileTypeCounts[typeIndex]++;
        }
    }

    printf("Total number of files: %d\n", totalFiles);
    printf("Total size of all files: %ld bytes\n", (long)totalSize);
    printf("File with the largest size: %s (%ld bytes)\n", maxFileSizeName, (long)maxFileSize);
    printf("File with the smallest size: %s (%ld bytes)\n", minFileSizeName, (long)minFileSize);

    // Display the number of each file type
    printf("Number of each file type:\n");
    for (int i = 0; i < MAX_FILE_TYPES && fileTypes[i] != NULL; i++) {
        printf("%s: %d\n", fileTypes[i], fileTypeCounts[i]);
    }
}


