#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>

#define MAX_EXTENSIONS 100
#define MAX_FILES 1000

typedef struct
{
    char extension[256];
    int count;
} ExtensionCount;

typedef struct
{
    char path[256];
    int smallest_size;
    char smallest_file[256];
    int largest_size;
    char largest_file[256];
    ExtensionCount extensions[MAX_EXTENSIONS];
    int extension_count;
} DirectoryData;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int total_files = 0;
int total_types = 0;
int root_directory_size = 0;

void exploreDirectory(const char *path);

void *exploreDirectoryThread(void *data);

void countExtensions(const char *filename, DirectoryData *dirData);

void updateSizes(const char *filename, DirectoryData *dirData);

void *exploreDirectoryThread(void *data)
{
    DirectoryData *dirData = (DirectoryData *)data;
    exploreDirectory(dirData->path);
    pthread_exit(NULL);
}

void exploreDirectory(const char *path)
{
    DIR *directory;
    struct dirent *entry;
    pthread_t threads[MAX_FILES];
    int thread_count = 0;
    DirectoryData *dirData = (DirectoryData *)malloc(sizeof(DirectoryData));
    strcpy(dirData->path, path);
    dirData->smallest_size = INT_MAX;
    dirData->largest_size = 0;
    dirData->extension_count = 0;

    directory = opendir(path);
    if (directory != NULL)
    {
        while ((entry = readdir(directory)) != NULL)
        {
            if (entry->d_type == DT_REG)
            { // Check if it's a regular file
                char filepath[256];
                snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);
                countExtensions(filepath, dirData);
                updateSizes(filepath, dirData);
                total_files++;
            }
            else if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            {
                char subdir[256];
                snprintf(subdir, sizeof(subdir), "%s/%s", path, entry->d_name);

                pthread_t tid;
                pthread_create(&tid, NULL, exploreDirectoryThread, (void *)subdir);
                pthread_join(tid, NULL);
            }
        }
        closedir(directory);

        pthread_mutex_lock(&mutex);

        printf("Directory: %s\n", dirData->path);
        for (int i = 0; i < dirData->extension_count; ++i)
        {
            printf("%s: %d\n", dirData->extensions[i].extension, dirData->extensions[i].count);
            total_types += dirData->extensions[i].count;
        }
        printf("Smallest File Size: %d bytes at %s\n", dirData->smallest_size, dirData->smallest_file);
        printf("Largest File Size: %d bytes at %s\n", dirData->largest_size, dirData->largest_file);
        printf("\n");

        root_directory_size += dirData->largest_size;

        pthread_mutex_unlock(&mutex);

        free(dirData);
    }
}

void countExtensions(const char *filename, DirectoryData *dirData)
{
    char *file_extension = strrchr(filename, '.');

    if (file_extension != NULL)
    {
        int found = 0;
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < dirData->extension_count; ++i)
        {
            if (strcmp(dirData->extensions[i].extension, file_extension + 1) == 0)
            {
                dirData->extensions[i].count++;
                found = 1;
                break;
            }
        }
        if (!found)
        {
            strcpy(dirData->extensions[dirData->extension_count].extension, file_extension + 1);
            dirData->extensions[dirData->extension_count].count = 1;
            dirData->extension_count++;
        }
        pthread_mutex_unlock(&mutex);
    }
}

void updateSizes(const char *filename, DirectoryData *dirData)
{
    struct stat st;
    if (stat(filename, &st) == 0)
    {
        int size = st.st_size;
        if (size < dirData->smallest_size)
        {
            dirData->smallest_size = size;
            strcpy(dirData->smallest_file, filename);
        }
        if (size > dirData->largest_size)
        {
            dirData->largest_size = size;
            strcpy(dirData->largest_file, filename);
        }
    }
}

int main()
{
    char directory_path[256];
    printf("Enter directory path: ");
    scanf("%s", directory_path);

    exploreDirectory(directory_path);

    printf("\nTotal number of files: %d\n", total_files);
    printf("Total number of file types: %d\n", total_types);
    printf("Root directory size: %d bytes\n", root_directory_size);

    return 0;
}
