#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <stdlib.h>

int totaltxtCount;
int totalpngCount;
int totaljpgCount;
int totalotherCount;
int totalfileCount;
pthread_mutex_t lock;

typedef struct {
    char directoryPath[100];
} ThreadArgs;

void calculateRootFolderSize(const char *directoryPath, uintmax_t *totalSize) {
    DIR *dir = opendir(directoryPath);
    if (dir == NULL) {
        printf("Failed to open directory.\n");
        return;
    }
    struct dirent *entry;
    struct stat fileStat;
    while ((entry = readdir(dir)) != NULL) {
        // Get the file path
        char filePath[150];
        strcpy(filePath, directoryPath);
        strcat(filePath, "/");
        strcat(filePath, entry->d_name);
        if (stat(filePath, &fileStat) < 0) {
            printf("Failed to get file information for %s\n", entry->d_name);
            continue;
        }
        if (S_ISREG(fileStat.st_mode)) {
            *totalSize += fileStat.st_size;
        }
        if (S_ISDIR(fileStat.st_mode) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            calculateRootFolderSize(filePath, totalSize);
        }
    }
    closedir(dir);
}

void findLargestAndSmallestFileSize(const char *directoryPath) {
    DIR *dir = opendir(directoryPath);
    if (dir == NULL) {
        printf("Failed to open directory.\n");
        return;
    }
    struct dirent *entry;
    struct stat fileStat;
    long long int largestSize = 0;
    long long int smallestSize = LLONG_MAX;
    char largestFilePath[150];
    char smallestFilePath[150];
    while ((entry = readdir(dir)) != NULL) {
        char filePath[150];
        strcpy(filePath, directoryPath);
        strcat(filePath, "/");
        strcat(filePath, entry->d_name);
        if (stat(filePath, &fileStat) < 0) {
            printf("Failed to get file information for %s\n", entry->d_name);
            continue;
        }
        if (S_ISREG(fileStat.st_mode)) {
            if (fileStat.st_size > largestSize) {
                largestSize = fileStat.st_size;
                strcpy(largestFilePath, filePath);
            }
            if (fileStat.st_size < smallestSize) {
                smallestSize = fileStat.st_size;
                strcpy(smallestFilePath, filePath);
            }
        }
    }
    printf("Largest file size: %lld bytes\n", largestSize);
    printf("Largest file path: %s\n", largestFilePath);
    printf("Smallest file size: %lld bytes\n", smallestSize);
    printf("Smallest file path: %s\n", smallestFilePath);
    closedir(dir);
}

void sharedLogic(const char directoryPath[100]) {
    struct dirent *entry;
    struct stat fileStat;
    DIR *dir = opendir(directoryPath);
    if (dir == NULL) {
        printf("Failed to open directory.\n");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        // Get the file path
        char filePath[150];
        strcpy(filePath, directoryPath);
        strcat(filePath, "/");
        strcat(filePath, entry->d_name);

        // Get the file type
        if (stat(filePath, &fileStat) < 0) {
            printf("Failed to get file information for %s\n", entry->d_name);
            continue;
        }
        if (S_ISREG(fileStat.st_mode)) {
            char *extension = strrchr(entry->d_name, '.');
            if (extension != NULL && strlen(extension) > 1) {
                if (strcmp(extension, ".txt") == 0) {
                    pthread_mutex_lock(&lock);
                    totaltxtCount++;
                    pthread_mutex_unlock(&lock);
                } else if (strcmp(extension, ".png") == 0) {
                    pthread_mutex_lock(&lock);
                    totalpngCount++;
                    pthread_mutex_unlock(&lock);
                } else if (strcmp(extension, ".jpg") == 0 || strcmp(extension, ".jpeg") == 0) {
                    pthread_mutex_lock(&lock);
                    totaljpgCount++;
                    pthread_mutex_unlock(&lock);
                } else {
                    pthread_mutex_lock(&lock);
                    totalotherCount++;
                    pthread_mutex_unlock(&lock);
                }
            } else {
                pthread_mutex_lock(&lock);
               totalotherCount++;
                pthread_mutex_unlock(&lock);
            }
        } else if (S_ISDIR(fileStat.st_mode) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            sharedLogic(filePath);
        }
    }
    closedir(dir);
}

void *calculateFileTypes(void *args) {
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    sharedLogic(threadArgs->directoryPath);
    pthread_exit(NULL);
}

int main() {
    totaltxtCount = 0;
    totalpngCount = 0;
    totaljpgCount = 0;
    totalotherCount = 0;
    totalfileCount = 0;

    char directoryPath[100];
    printf("Enter the directory path: ");
    scanf("%s", directoryPath);
    sharedLogic(directoryPath);

    // Calculate the total file count and print the results
    calculateRootFolderSize(directoryPath, &totalfileCount);
    printf("Total .txt files: %d\n", totaltxtCount);
    printf("Total .png files: %d\n", totalpngCount);
    printf("Total .jpg files: %d\n", totaljpgCount);
    printf("Total other files: %d\n", totalotherCount);
    printf("Total files: %d\n", totalfileCount);

    findLargestAndSmallestFileSize(directoryPath);

    return 0;
}