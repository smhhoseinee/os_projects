#define _XOPEN_SOURCE 700
#define _GNU_SOURCE

#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>

#define MAX_PATH_LENGTH 1024

void printFileInfo(const char *dir_path, const char *file_name);
void printExtensionCounts();
void printMinMaxFileSize();
const char *formatSize(long long size);
void printTotalSize();

struct ExtensionCount
{
    char extension[MAX_PATH_LENGTH];
    int count;
};

#define MAX_EXTENSIONS 100
struct ExtensionCount extensions[MAX_EXTENSIONS];
int extensionCount = 0;

pthread_mutex_t countMutex = PTHREAD_MUTEX_INITIALIZER;

long long minFileSize = LLONG_MAX;
long long maxFileSize = LLONG_MIN;
long long totalSize = 0;
char minFilePath[MAX_PATH_LENGTH];
char maxFilePath[MAX_PATH_LENGTH];

int main(int argc, char *argv[])
{
    const char *directory_path;

    if (argc == 1)
    {
        directory_path = ".";
    }
    else if (argc == 2)
    {
        directory_path = argv[1];
    }
    else
    {
        fprintf(stderr, "Usage: %s [directory_path]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    listFiles(directory_path);

    printExtensionCounts();
    printMinMaxFileSize();
    printTotalSize();

    return 0;
}


void printFileInfo(const char *dir_path, const char *file_name)
{
    struct stat file_stat;

    char file_path[MAX_PATH_LENGTH];
    snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, file_name);

    if (lstat(file_path, &file_stat) == 0)
    {
        if (S_ISREG(file_stat.st_mode))
        {
            //printf("%s/%s - Size: %s\n", dir_path, file_name, formatSize(file_stat.st_size));
            pthread_mutex_lock(&countMutex);

            if (file_stat.st_size < minFileSize)
            {
                minFileSize = file_stat.st_size;
                strncpy(minFilePath, file_path, sizeof(minFilePath));
            }

            if (file_stat.st_size > maxFileSize)
            {
                maxFileSize = file_stat.st_size;
                strncpy(maxFilePath, file_path, sizeof(maxFilePath));
            }

            pthread_mutex_unlock(&countMutex);

            const char *extension = strrchr(file_name, '.');
            if (extension == NULL){
                extension = "executable";
            }
            if (extension != NULL)
            {
                extension++;

                pthread_mutex_lock(&countMutex);

                int found = 0;
                for (int i = 0; i < extensionCount; ++i)
                {
                    if (strcmp(extensions[i].extension, extension) == 0)
                    {
                        extensions[i].count++;
                        found = 1;
                        break;
                    }
                }
                if (!found)
                {
                    if (extensionCount < MAX_EXTENSIONS)
                    {
                        strncpy(extensions[extensionCount].extension, extension, sizeof(extensions[extensionCount].extension));
                        extensions[extensionCount].count = 1;
                        extensionCount++;
                    }
                }
                totalSize += file_stat.st_size;
                pthread_mutex_unlock(&countMutex);
            }
        }
    }
    else
    {
        //perror("lstat");
    }
}

void listFiles(const char *dir_path)
{
    DIR *dir = opendir(dir_path);
    if (dir == NULL)
    {
        perror("opendir");
        exit(EXIT_FAILURE);
    }
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            char subdirectory_path[MAX_PATH_LENGTH];
            snprintf(subdirectory_path, sizeof(subdirectory_path), "%s/%s", dir_path, entry->d_name);
            
            pid_t pid = fork(); // Fork a new process

            if (pid == -1)
            {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            else if (pid == 0)
            {
                // Child process
                listFiles(subdirectory_path); // Recursively list files in the subdirectory
                exit(EXIT_SUCCESS); // Exit the child process
            }
            else
            {
                // Parent process
                pthread_t tid;
                if (pthread_create(&tid, NULL, processDirectory, strdup(subdirectory_path)) != 0)
                {
                    perror("pthread_create");
                    exit(EXIT_FAILURE);
                }

                if (pthread_join(tid, NULL) != 0)
                {
                    perror("pthread_join");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    closedir(dir);
}

void *processDirectory(void *param)
{
    const char *dir_path = (const char *)param;
    DIR *dir = opendir(dir_path);

    if (dir == NULL)
    {
        perror("opendir");
        pthread_exit(NULL);
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG)
        {
            char file_path[MAX_PATH_LENGTH];
            snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry->d_name);
            printFileInfo(dir_path, entry->d_name);
            printFileInfo(file_path, entry->d_name);
        }
        else if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            char subdirectory_path[MAX_PATH_LENGTH];
            snprintf(subdirectory_path, sizeof(subdirectory_path), "%s/%s", dir_path, entry->d_name);

            pthread_t tid;
            if (pthread_create(&tid, NULL, processDirectory, strdup(subdirectory_path)) != 0)
            {
                perror("pthread_create");
                closedir(dir);
                pthread_exit(NULL);
            }

            if (pthread_join(tid, NULL) != 0)
            {
                perror("pthread_join");
                closedir(dir);
                pthread_exit(NULL);
            }
        }
    }

    closedir(dir);
    pthread_exit(NULL);
}


void printExtensionCounts()
{
    FILE *file = fopen("extension_counts.txt", "w");
    if (file == NULL)
    {
        perror("fopen");
        return;
    }

    pthread_mutex_lock(&countMutex);

    for (int i = 0; i < extensionCount; ++i)
    {
        fprintf(file, "%s %d\n", extensions[i].extension, extensions[i].count);
    }

    printf("\nNumber of each file type:\n");
    for (int i = 0; i < extensionCount; ++i)
    {
        printf("- .%s: %d\n", extensions[i].extension, extensions[i].count);
    }

    pthread_mutex_unlock(&countMutex);

    fclose(file);
}

void printMinMaxFileSize()
{
    pthread_mutex_lock(&countMutex);

    printf("\nMinimum File Size: %s (Path: %s)\n", formatSize(minFileSize), minFilePath);
    printf("Maximum File Size: %s (Path: %s)\n", formatSize(maxFileSize), maxFilePath);

    pthread_mutex_unlock(&countMutex);
}

const char *formatSize(long long size)
{
    static const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;

    while (size > 1000 && unitIndex < sizeof(units) / sizeof(units[0]) - 1)
    {
        size /= 1000;
        ++unitIndex;
    }

    static char result[20];
    snprintf(result, sizeof(result), "%lld %s", size, units[unitIndex]);

    return result;
}

void printTotalSize()
{
    pthread_mutex_lock(&countMutex);

    printf("\nTotal Directory Size: %s\n", formatSize(totalSize));

    pthread_mutex_unlock(&countMutex);
}