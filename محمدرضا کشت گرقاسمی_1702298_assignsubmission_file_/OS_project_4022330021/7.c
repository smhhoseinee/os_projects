#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <mqueue.h>

#define MAX_TYPES 100
#define MAX_PATH_LENGTH 256
#define MAX_MESSAGE_SIZE 256

struct Message {
    long messageType;
    char messageText[MAX_MESSAGE_SIZE];
};

char path_of_largest_file[MAX_PATH_LENGTH];
char path_of_smallest_file[MAX_PATH_LENGTH];
off_t size_of_largest_file = 0;
off_t size_of_smallest_file = LLONG_MAX;
int number_of_files = 0;
char types[MAX_TYPES][10];
int type_counts[MAX_TYPES] = {0};

void exploring(const char *directory_path, const char *root_path);
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

char path_of_user_entered[MAX_PATH_LENGTH];

void *exploreSubdirectories(void *lpParam) {
    
    char *subdirectory_path = (char *)lpParam;
    pthread_mutex_lock(&mutex);
    char root_path[MAX_PATH_LENGTH]; 
    strcpy(root_path, path_of_user_entered); 
    pthread_mutex_unlock(&mutex);
    exploring(subdirectory_path,path_of_user_entered);

    free(subdirectory_path);
    return NULL;
}


void exploring(const char *directory_path, const char *root_path) {
    DIR *dir;
    struct dirent *entry;
    char full_path[MAX_PATH_LENGTH];

    dir = opendir(directory_path);

    if (dir == NULL) {
        fprintf(stderr, "Error opening directory: %s\n", directory_path);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            number_of_files++;

            char *extension = strrchr(entry->d_name, '.');
            if (extension != NULL) {
                int found = 0;
                char ext[10];
                strcpy(ext, extension);
                for (int i = 0; ext[i]; i++) {
                    ext[i] = tolower(ext[i]);
                }
                for (int i = 0; i < MAX_TYPES; i++) {
                    if (types[i][0] == '\0') {
                        pthread_mutex_lock(&mutex);

                        strcpy(types[i], ext);
                        type_counts[i]++;
                        pthread_mutex_unlock(&mutex);

                        found = 1;
                        break;

                    } else if (strcmp(types[i], ext) == 0) {
                        pthread_mutex_lock(&mutex);

                        type_counts[i]++;
                        pthread_mutex_unlock(&mutex);

                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    printf("Too many file types found. Increase MAX_TYPES.\n");
                    return;
                }
            }

            char full_entry_path[MAX_PATH_LENGTH];
            sprintf(full_entry_path, "%s/%s", directory_path, entry->d_name);

            struct stat stat_buf;
            
            if (lstat(full_entry_path, &stat_buf) == -1) {

                fprintf(stderr, "Error getting file stats: %s\n", full_entry_path);
                continue;
            }


            off_t file_size = stat_buf.st_size;

            if (S_ISREG(stat_buf.st_mode)) {
                pthread_mutex_lock(&mutex);

                if (file_size > size_of_largest_file) {
                    size_of_largest_file = file_size;
                    strcpy(path_of_largest_file, full_entry_path);
                }

                if (file_size < size_of_smallest_file) {
                    size_of_smallest_file = file_size;
                    strcpy(path_of_smallest_file, full_entry_path);
                }
                pthread_mutex_unlock(&mutex);

            }
        }

        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char full_entry_path[MAX_PATH_LENGTH];
            sprintf(full_entry_path, "%s/%s", directory_path, entry->d_name);

            struct stat stat_buf;
            if (lstat(full_entry_path, &stat_buf) == -1) {
                fprintf(stderr, "Error getting file stats: %s\n", full_entry_path);
                continue;
            }

            if (S_ISDIR(stat_buf.st_mode)) {
                char resolved_path[MAX_PATH_LENGTH];
                realpath(full_entry_path, resolved_path);/
                if (strstr(resolved_path, root_path) != NULL) {
                   
                    pid_t pid = fork();

                    if (pid == -1) {
                        printf("Error creating process for directory exploration.\n");
                        continue;
                    }

                    if (pid == 0) {
                        exploring(full_entry_path, root_path);
                        exit(0);
                    }
                    else {
                        wait(NULL);
                        pthread_t thread;
                        if (pthread_create(&thread, NULL, exploreSubdirectories, strdup(full_entry_path)) != 0) {
                            printf("Error creating thread for subdirectory exploration.\n");
                        } else {
                            pthread_join(thread, NULL);
                        }
                    }
                } else {
                    pthread_t thread;
                    if (pthread_create(&thread, NULL, exploreSubdirectories, strdup(full_entry_path)) != 0) {
                        printf("Error creating thread for subdirectory exploration.\n");
                    } else {
                        pthread_join(thread, NULL);
                    }
                }
            }
        }
    }

    closedir(dir);
}



void getPathFromUser() {/
    printf("Enter the directory path: ");
    scanf("%s", path_of_user_entered);
}

void formatSize(off_t size, char *formattedSize) {
    if (size < 1024) {
        sprintf(formattedSize, "%lld bytes", (long long)size);
    } else if (size < 1024 * 1024) {
        sprintf(formattedSize, "%.2f KB", (double)size / 1024);
    } else if (size < 1024 * 1024 * 1024) {
        sprintf(formattedSize, "%.2f MB", (double)size / (1024 * 1024));
    } else {
        sprintf(formattedSize, "%.2f GB", (double)size / (1024 * 1024 * 1024));
    }
}

off_t size_of_root_directory = 0;

void getSizeOfDirectory(const char *directory_path) {
   DIR *dir;
    struct dirent *entry;

    dir = opendir(directory_path);

    if (dir == NULL) {
        fprintf(stderr, "Error opening directory: %s\n", directory_path);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char full_entry_path[MAX_PATH_LENGTH];
            sprintf(full_entry_path, "%s/%s", directory_path, entry->d_name);
            getSizeOfDirectory(full_entry_path);
        }

        struct stat stat_buf;
        char full_entry_path[MAX_PATH_LENGTH];
        sprintf(full_entry_path, "%s/%s", directory_path, entry->d_name);

        if (lstat(full_entry_path, &stat_buf) == -1) {

            fprintf(stderr, "Error getting file stats: %s\n", full_entry_path);
            continue;
        }

        off_t file_size = stat_buf.st_size;
        size_of_root_directory += file_size;
    }

    closedir(dir);
}

int main() {
    getPathFromUser();
    printf("\nExploring directory...\n\n");

    exploring(path_of_user_entered,path_of_user_entered);
    getSizeOfDirectory(path_of_user_entered);

    printf("Results:\n");
    printf("Total number of files: %d\n", number_of_files);
    printf("Number of each file type:\n");
    for (int i = 0; i < MAX_TYPES; i++) {
        if (types[i][0] == '\0')
            break;
        printf("-%s: %d\n", types[i], type_counts[i]);
    }

    char formattedLargestSize[20];
    char formattedSmallestSize[20];
    formatSize(size_of_largest_file, formattedLargestSize);
    formatSize(size_of_smallest_file, formattedSmallestSize);

    printf("File with the largest size: %s, Size: %s\n", path_of_largest_file, formattedLargestSize);
    printf("File with the smallest size: %s, Size: %s\n", path_of_smallest_file, formattedSmallestSize);

    char formattedRootSize[20];
    formatSize(size_of_root_directory, formattedRootSize);
    printf("Size of the root directory: %s\n", formattedRootSize);

    return 0;
}