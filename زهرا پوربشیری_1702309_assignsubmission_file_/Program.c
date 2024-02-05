#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>

#define MAX_FILE_TYPES 100
#define MAX_PATH_LENGTH 1024

struct FileStats {
    int total_files;
    int file_types[MAX_FILE_TYPES];
    char largest_file_path[MAX_PATH_LENGTH];
    char smallest_file_path[MAX_PATH_LENGTH];
    long long largest_file_size;
    long long smallest_file_size;
    long long root_folder_size;
};

pthread_mutex_t lock;

char *file_type_extensions[MAX_FILE_TYPES] = {
    "txt", "pdf", "jpg", "png", "c", "py", "git", "js","out","v","cpp","json" /* Add more extensions as needed */
};

int is_known_extension(const char *file_name) {
    char *file_extension = strrchr(file_name, '.');
    if (file_extension != NULL) {
        file_extension++;  // Skip the dot in the file extension

        for (int i = 0; i < MAX_FILE_TYPES; i++) {
            if (file_type_extensions[i] != NULL && strcmp(file_extension, file_type_extensions[i]) == 0) {
                return 1; // Known extension
            }
        }
    }
    return 0; // Unknown extension
}

void process_folder(char *path, struct FileStats *stats) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;

    dir = opendir(path);

    if (!dir) {
        perror("Error opening directory");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        char file_path[MAX_PATH_LENGTH];
        snprintf(file_path, sizeof(file_path), "%s/%s", path, entry->d_name);

        if (stat(file_path, &file_stat) == -1) {
            perror("Error getting file information");
            continue;
        }

        if (S_ISDIR(file_stat.st_mode)) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                // Recursive call for subdirectories
                process_folder(file_path, stats);
            }
        } else if (S_ISREG(file_stat.st_mode)) {
            // Update statistics using a lock to ensure synchronization
            pthread_mutex_lock(&lock);
            stats->total_files++;

            // Update file types count based on known extensions
            if (is_known_extension(entry->d_name)) {
                for (int i = 0; i < MAX_FILE_TYPES; i++) {
                    if (file_type_extensions[i] != NULL && strcmp(file_type_extensions[i], entry->d_name + strlen(entry->d_name) - strlen(file_type_extensions[i])) == 0) {
                        stats->file_types[i]++;
                        break;
                    }
                }
            }

            // Update largest file
            if (file_stat.st_size > stats->largest_file_size) {
                strcpy(stats->largest_file_path, file_path);
                stats->largest_file_size = file_stat.st_size;
            }

            // Update smallest file
            if (file_stat.st_size < stats->smallest_file_size || stats->smallest_file_size == 0) {
                strcpy(stats->smallest_file_path, file_path);
                stats->smallest_file_size = file_stat.st_size;
            }

            // Update root folder size
            stats->root_folder_size += file_stat.st_size;

            pthread_mutex_unlock(&lock);
        }
    }

    closedir(dir);
}

void *thread_function(void *arg) {
    char *path = (char *)arg;
    struct FileStats *stats = malloc(sizeof(struct FileStats));
    memset(stats, 0, sizeof(struct FileStats));

    process_folder(path, stats);

    pthread_exit(stats);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *directory_path = argv[1];

    pthread_mutex_init(&lock, NULL);

    pthread_t thread;
    if (pthread_create(&thread, NULL, thread_function, directory_path) != 0) {
        perror("Error creating thread");
        exit(EXIT_FAILURE);
    }

    struct FileStats *stats;
    if (pthread_join(thread, (void **)&stats) != 0) {
        perror("Error joining thread");
        exit(EXIT_FAILURE);
    }

    // Display the results
    printf("Total number of files: %d\n", stats->total_files);

    printf("Number of each file type:\n");
    for (int i = 0; i < MAX_FILE_TYPES; i++) {
        if (file_type_extensions[i] != NULL && stats->file_types[i] > 0) {
            printf(".%s: %d\n", file_type_extensions[i], stats->file_types[i]);
        }
    }

    printf("File with the largest size: %s, Size: %lld bytes\n", stats->largest_file_path, stats->largest_file_size);
    printf("File with the smallest size: %s, Size: %lld bytes\n", stats->smallest_file_path, stats->smallest_file_size);
    printf("Size of the root folder: %lld bytes\n", stats->root_folder_size);

    free(stats);
    pthread_mutex_destroy(&lock);

    return 0;
}
