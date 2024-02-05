#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>

// Define a constant for the maximum length of the file path
#define MAX_PATH_LENGTH 512

// Define a constant for the maximum number of unique file types
#define MAX_FILE_TYPES 100

// Define the struct for folder information
struct FolderInfo {
    int fileCount;
    off_t maxFileSize;
    off_t minFileSize;
    char maxFilePath[MAX_PATH_LENGTH];
    char minFilePath[MAX_PATH_LENGTH];
    off_t folderSize;
    struct FileTypeCount {
        char* extension;
        int count;
    } fileTypeCounts[MAX_FILE_TYPES];
    int numFileTypes;
                char* extension = strrchr(entry->d_name, '.');
                if (extension != NULL) {
                    int found = -1;
                    for (int i = 0; i < threadArgs->info->numFileTypes; ++i) {
                        if (strcmp(threadArgs->info->fileTypeCounts[i].extension, extension) == 0) {
                            found = i;
                            break;
                        }
                    }

                    if (found != -1) {
                        threadArgs->info->fileTypeCounts[found].count++;
                    } else {
                        if (threadArgs->info->numFileTypes < MAX_FILE_TYPES) {
                            threadArgs->info->fileTypeCounts[threadArgs->info->numFileTypes].extension = strdup(extension);
                            threadArgs->info->fileTypeCounts[threadArgs->info->numFileTypes].count = 1;
                            threadArgs->info->numFileTypes++;
                        }
                    }
                }
            } else if (S_ISDIR(fileStat.st_mode) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                // If entry is a directory (and not '.' or '..'), recursively process the subdirectory
                struct ThreadArgs subDirArgs;
                snprintf(subDirArgs.folderPath, sizeof(subDirArgs.folderPath), "%s/%s", threadArgs->folderPath, entry->d_name);
                subDirArgs.info = threadArgs->info;

                // Create a thread for the subdirectory
                pthread_t subDirThread;
                pthread_create(&subDirThread, NULL, processFolder, &subDirArgs);

                // Wait for the subdirectory thread to finish
                pthread_join(subDirThread, NULL);
            }
        } else {
            perror("Unable to get file status.");
        }
    }
   
    // Close the directory
    closedir(dir);
    return NULL;
}

// Function to get subfolder paths in level 1
char** getLevelOneSubfolders(const char *folderPath, int *numSubfolders) {
    DIR *dir = opendir(folderPath);
    if (dir == NULL) {
        perror("Unable to open directory.");
        return NULL;
    }

    // Initialize variables
    int capacity = 10;
    *numSubfolders = 0;
    char **subfolders = (char **)malloc(capacity * sizeof(char *));
    struct dirent *entry;

    // Iterate through the directory entries
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            // Ignore "." and ".."
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                // Construct the complete subfolder path
                char subfolderPath[MAX_PATH_LENGTH];
                snprintf(subfolderPath, sizeof(subfolderPath), "%s/%s", folderPath, entry->d_name);

                // Allocate memory for the subfolder path
                subfolders[*numSubfolders] = strdup(subfolderPath);
                (*numSubfolders)++;

                // Resize the array if necessary
                if (*numSubfolders >= capacity) {
                    capacity *= 2;
                    subfolders = (char **)realloc(subfolders, capacity * sizeof(char *));
                }
            }
        }
    }

    closedir(dir);

    return subfolders;
}

int main() {
    char path[MAX_PATH_LENGTH];
    printf("Enter the directory path: ");
    scanf("%s", path);

    struct FolderInfo pathInfo = {0, 0, -1, "", "", 0, {0}, 0};

    // Create thread arguments for the input path
    struct ThreadArgs pathArgs;
    strcpy(pathArgs.folderPath, path);
    pathArgs.info = &pathInfo;

    // Create a thread for the input path
    pthread_t pathThread;
    pthread_create(&pathThread, NULL, processFolder, &pathArgs);

    // Wait for the input path thread to finish
    pthread_join(pathThread, NULL);

    // Print the results for the input path
    printf("Total number of all files: %d\n", pathInfo.fileCount);
    for (int i = 0; i < pathInfo.numFileTypes; ++i) {
        printf("File type %s: %d files\n", pathInfo.fileTypeCounts[i].extension, pathInfo.fileTypeCounts[i].count);
    }
    printf("File with the largest size: %s (%ld bytes)\n", pathInfo.maxFilePath, (long) pathInfo.maxFileSize);
    printf("File with the smallest size: %s (%ld bytes)\n", pathInfo.minFilePath, (long) pathInfo.minFileSize);
    printf("Size of the input path: %ld bytes\n", (long) pathInfo.folderSize);

    // Free dynamically allocated file type extensions
    for (int i = 0; i < pathInfo.numFileTypes; ++i) {
        free(pathInfo.fileTypeCounts[i].extension);
    }

    return 0;
}