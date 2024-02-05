#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>

#define MAX 1024

typedef struct {
    char filename[MAX];
    off_t size;
} FileInfo;

typedef struct {
    char extension[10];
    int count;
}FileType ;

struct TotalInfo {
    char path[MAX];
    FileInfo file_info[MAX];
    FileInfo largest_file;
    FileInfo smallest_file;
    long total_files;
    int file_type_count;
    off_t total_size;
    int smallest_flag;
    pthread_mutex_t mutex;
    FileType fileTypes[MAX];
};

typedef struct {
    char path[MAX];
    struct TotalInfo* TotalInfo;
} FolderInfo;

void process_directory(const char *dir, struct TotalInfo *shared_data);
void *thread_directory(void *arg);
void add_extension(FileType* fType, char* type, int* len);
char *get_extension(const char *filePath);


int main(int argc, char *argv[]){
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int shared_memory_fd = shm_open("/shared_data", O_CREAT | O_RDWR, S_IRWXU);
    ftruncate(shared_memory_fd, sizeof(struct TotalInfo));
    struct TotalInfo *shared_data = mmap(NULL, sizeof(struct TotalInfo), PROT_READ | PROT_WRITE, MAP_SHARED, shared_memory_fd, 0);
    strcpy(shared_data->path, argv[1]);
    pthread_mutex_init(&shared_data->mutex, NULL);
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    dir = opendir(shared_data->path);
    if (dir == NULL) {
        fprintf(stderr, "cannot open directory %s\n", shared_data->path);
        exit(EXIT_FAILURE);
    }
    while ((entry = readdir(dir)) != NULL) {
        char file_path[MAX];
        snprintf(file_path, sizeof(file_path), "%s/%s", shared_data->path, entry->d_name);
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            pid_t pid = fork();
            if (pid == 0) {
                char child_path[1024];
                snprintf(child_path, sizeof(child_path), "%s/%s", shared_data->path, entry->d_name);
                printf("Process %d is processing directory: %s\n", getpid(), child_path);
                process_directory(child_path, shared_data);
                closedir(dir);
                exit(EXIT_SUCCESS);
            }
        }
        else if (stat(file_path, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
            pthread_mutex_lock(&shared_data->mutex);
            if (shared_data->total_files < MAX) {
                FileInfo temp;
                temp.size = file_stat.st_size;
                strcpy(temp.filename, file_path);
                memcpy(&shared_data->file_info[shared_data->total_files], &temp, sizeof(FileInfo));
            }
            shared_data->total_files++;
            shared_data->total_size += file_stat.st_size;
            if (file_stat.st_size > shared_data->largest_file.size) {
                shared_data->largest_file.size = file_stat.st_size;
                strcpy(shared_data->largest_file.filename, file_path);
            }
            if (!shared_data->smallest_flag || file_stat.st_size < shared_data->smallest_file.size) {
                shared_data->smallest_file.size = file_stat.st_size;
                strcpy(shared_data->smallest_file.filename, file_path);
                shared_data->smallest_flag = 1;
            }
            add_extension(shared_data->fileTypes, get_extension(file_path), &shared_data->file_type_count);
            pthread_mutex_unlock(&shared_data->mutex);
        }
    }
    while(wait(NULL) != -1);
    closedir(dir);
    printf("Total number of files: %ld\n", shared_data->total_files);
    printf("Largest file: %s, ( %ld bytes )\n", shared_data->largest_file.filename, shared_data->largest_file.size);
    printf("Smallest file: %s, ( %ld bytes )\n", shared_data->smallest_file.filename, shared_data->smallest_file.size);
    printf("Total size: %ld bytes\n", shared_data->total_size);
    for (int i = 0; i < shared_data->file_type_count; i++) {
        printf("%s: %d\n", shared_data->fileTypes[i].extension, shared_data->fileTypes[i].count);
    }
    munmap(shared_data, sizeof(struct TotalInfo));
    close(shared_memory_fd);
    shm_unlink("/shared_data");
    pthread_mutex_destroy(&shared_data->mutex);
}


void process_directory(const char *directory, struct TotalInfo *shared_data) {
    DIR *dir;
    struct dirent *entry;
    dir = opendir(directory);
    if (!dir) {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }
    pthread_t threads[MAX];
    FolderInfo *folderInfo = malloc(1024 * sizeof(FolderInfo));
    if (!folderInfo) {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }
    int folder_counter = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        snprintf(folderInfo[folder_counter].path, sizeof(folderInfo[folder_counter].path), "%s/%s", directory, entry->d_name);
        folderInfo[folder_counter].TotalInfo = shared_data;
        if (entry->d_type == DT_DIR) {
            if (pthread_create(&threads[folder_counter], NULL, thread_directory, &folderInfo[folder_counter]) != 0) {
                perror("Error creating thread");
                exit(EXIT_FAILURE);
            }
            folder_counter++;
        } else {
            struct stat file_stat;
            char file_path[MAX];
            snprintf(file_path, sizeof(file_path), "%s", folderInfo[folder_counter].path);
            if (stat(file_path, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
                pthread_mutex_lock(&shared_data->mutex);
                if (shared_data->total_files < MAX) {
                    FileInfo temp;
                    temp.size = file_stat.st_size;
                    strcpy(temp.filename, file_path);
                    memcpy(&shared_data->file_info[shared_data->total_files], &temp, sizeof(FileInfo));
                }
                shared_data->total_files++;
                shared_data->total_size += file_stat.st_size;
                if (file_stat.st_size > shared_data->largest_file.size) {
                    shared_data->largest_file.size = file_stat.st_size;
                    strcpy(shared_data->largest_file.filename, file_path);
                }
                if (!shared_data->smallest_flag || file_stat.st_size < shared_data->smallest_file.size) {
                    shared_data->smallest_file.size = file_stat.st_size;
                    strcpy(shared_data->smallest_file.filename, file_path);
                    shared_data->smallest_flag = 1;  // Set the flag
                }
                add_extension(shared_data->fileTypes, get_extension(file_path), &shared_data->file_type_count);
                pthread_mutex_unlock(&shared_data->mutex);
            }
        }
    }
    for (int i = 0; i < folder_counter; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Error joining thread");
            exit(EXIT_FAILURE);
        }
    }
    free(folderInfo);
}

void *thread_directory(void *arg) {
    FolderInfo *folderInfo = (FolderInfo *)arg;
    pthread_t tid = pthread_self();
    printf("Thread %lu is processing directory: %s\n", (unsigned long)tid, folderInfo->path);
    process_directory(folderInfo->path, folderInfo->TotalInfo);
    return NULL;
}


void add_extension(FileType* fType, char* type, int* len){
    for(int i = 0; i  < (*len); i++){
        if(strcmp(fType[i].extension, type) == 0){
            fType[i].count++;
            return;
        }
    }
    FileType ft;
    ft.count = 1;
    strcpy(ft.extension, type);
    fType[*len] = ft;
    (*len)++;
}

char *get_extension(const char *filePath){
    char *splitName = strrchr(filePath, '.');
    if(!splitName || splitName == filePath)
        return "";
    return splitName + 1;
}