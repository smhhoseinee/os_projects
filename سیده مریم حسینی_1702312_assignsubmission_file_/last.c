#define _GNU_SOURCE
#include <pthread.h>
#include <bits/pthreadtypes.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <limits.h>
#include <linux/limits.h>
#include <semaphore.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*struct FileData {
    char path[256];
    long size;
};*/

// Define a structure to store monitoring data
typedef struct  {
    int totalFiles;
    int fileFormats[256];  // Assuming a maximum of 256 different file formats
    long int maxSize;
    long int minSize;
    off_t rootFolderSize;
}MonitoringData;

MonitoringData m1;

void* processFolder(void* folderPath) ;
void* process(void* path);
void createProcess(const char *directory);

int main() {
    char rootFolder[256];

    // Get user input for the root folder
    printf("Enter the path of the root folder: ");
    scanf("%s", rootFolder);

    // Initialize monitoring data
   
    //m1.totalFiles = 0;

    (m1.totalFiles) = 0;
    
    for (int i = 0; i < 256; i++) {
        m1.fileFormats[i] = 0;
    }
    m1.maxSize = LONG_MIN;
    m1.minSize = -1;
    m1.rootFolderSize = 0;

    // Process the root folder
    //processFolder(rootFolder);
    createProcess(rootFolder);
    pthread_t thread;
    pthread_create(&thread, NULL, processFolder, (void*) &rootFolder);
    pthread_join(thread, NULL);

    // Print monitoring results
    printf("Total number of files: %d\n", m1.totalFiles);
    printf("Number of each file format:\n");
    for (int i = 0; i < 256; i++) {
        if (m1.fileFormats[i] > 0) {
            printf("- Format %d: %d\n", i, m1.fileFormats[i]);
        }
    }
    printf("Maximum size: %ld bytes\n", m1.maxSize);
    printf("Minimum size: %ld bytes\n", m1.minSize);
    printf("Size of root folder: %ld bytes\n", m1.rootFolderSize);

    return 0;
}


void* processFolder(void* path) {
    DIR* dir;
    struct dirent* entry;
    struct stat fileStat;

    char* folderPath = (char*) path;
    dir = opendir(folderPath);

    /*if (dir == NULL) {
        perror("Error opening directory");
        return;
    }*/
    //struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {

        if (strcmp(entry-> d_name, ".")== 0 || strcmp(entry-> d_name, "..")== 0)
            continue;

        if (entry->d_type == DT_REG) {
            char filePath[256];
            //snprintf(filePath, sizeof(filePath), "%s/%s", folderPath, entry->d_name);
            printf("file: %s\n", entry->d_name);

            if (stat(filePath, &fileStat) == 0) {
                pthread_mutex_lock(&mutex);

                // Update monitoring data
                (m1.totalFiles)++;
                m1.fileFormats[entry->d_type]++;

                if (fileStat.st_size > m1.maxSize) {
                    m1.maxSize = fileStat.st_size;
                }

                if (fileStat.st_size < m1.minSize || m1.minSize == -1) {
                    m1.minSize = fileStat.st_size;
                }

                m1.rootFolderSize += fileStat.st_size;

                pthread_mutex_unlock(&mutex);
            } else {
                perror("Error getting file stat");
            }
        } else if (entry->d_type == DT_DIR) {
            char subFolderPath[256];
            //snprintf(subFolderPath, sizeof(subFolderPath), "%s/%s", folderPath, entry->d_name);
            printf("dir: %s\n", entry->d_name);
            // Process each subfolder recursively
            /////////////////////////////////////
            pthread_t thread1;
            pthread_create(&thread1, NULL, processFolder, (void*) &subFolderPath);
            pthread_join(thread1, NULL);

            //processFolder(subFolderPath);
        }
    }

    closedir(dir);
}


void* process(void* path) {
    DIR* dir;
    struct dirent* entry;
    struct stat fileStat;

    char* folderPath = (char*) path;
    dir = opendir(folderPath);

    /*if (dir == NULL) {
        perror("Error opening directory");
        return;
    }*/
    //struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {

        if (strcmp(entry-> d_name, ".")== 0 || strcmp(entry-> d_name, "..")== 0)
            continue;

        if (entry->d_type == DT_REG) {
            char filePath[256];
            //snprintf(filePath, sizeof(filePath), "%s/%s", folderPath, entry->d_name);
            printf("file: %s\n", entry->d_name);

            if (stat(filePath, &fileStat) == 0) {
                pthread_mutex_lock(&mutex);

                // Update monitoring data
                (m1.totalFiles)++;
                m1.fileFormats[entry->d_type]++;

                if (fileStat.st_size > m1.maxSize) {
                    m1.maxSize = fileStat.st_size;
                }

                if (fileStat.st_size < m1.minSize || m1.minSize == -1) {
                    m1.minSize = fileStat.st_size;
                }

                m1.rootFolderSize += fileStat.st_size;

                pthread_mutex_unlock(&mutex);
            } else {
                perror("Error getting file stat");
            }
        } else if (entry->d_type == DT_DIR) {
            char subFolderPath[256];
            //snprintf(subFolderPath, sizeof(subFolderPath), "%s/%s", folderPath, entry->d_name);
            printf("dir: %s\n", entry->d_name);
            // Process each subfolder recursively
            /////////////////////////////////////
        
            processFolder(subFolderPath);
        }
    }

    closedir(dir);
}

void createProcess(const char *directory){
    pid_t pid = fork();

    if (pid < 0){
        fprintf(stderr, "Error creating process\n");
        exit(EXIT_FAILURE);
    } else if (pid == 0){
        printf("Running process in %s\n", directory);
        exit(EXIT_SUCCESS);
    } else {
        int status;
        wait(&status);
    }
}
