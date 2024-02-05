#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <limits.h>
#include <semaphore.h>


#define MAX_PATH_LEN 100000
#define MAX_FILE_TYPES 20
#define FILE_TYPE 10


// Structure to store file information

struct FileTypes {
    int num;
    char type[FILE_TYPE];
};

struct FileInfo {
    int fileCount;
    struct FileTypes types[MAX_FILE_TYPES];
    int filetype_num;
    long long totalSize;
    int max_size;
    int min_size;
    char largestFile[MAX_PATH_LEN];
    char smallestFile[MAX_PATH_LEN];
};

struct ThreadArgs {
    char directory[MAX_PATH_LEN];
};


// Global variables for shared memory and mutex and semaphore
int shmid;
struct FileInfo* sharedData;
pthread_mutex_t lock;
sem_t semaphore;

void *traverseWithThread(char *directory);

int traverse(char *directory);

void *threadFunc(void *arg); 

char *getTypeOfFile(char name[]);

int findFileType(struct FileTypes filetypes[], char t[]);

void printFileInfo();

int main() {

    char *startPath = ".";
    //sprintf("please enter address of directory: %s", &startPath);
    
      shmid = shmget(IPC_PRIVATE, sizeof(struct FileInfo), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory
    sharedData = (struct FileInfo*)shmat(shmid, NULL, 0);
    if (sharedData == (void *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    sharedData->max_size = INT_MIN;
    sharedData->min_size = INT_MAX;


    sem_init(&semaphore, 0, 1);
    pthread_mutex_init(&lock, NULL);
    traverse(startPath);

    shmdt(sharedData);

    sem_destroy(&semaphore);
    pthread_mutex_destroy(&lock);
    return 0;
}

int traverse(char *directory) {
    
    DIR *cur_dir = opendir(directory);
    if (!cur_dir) {
        printf("COULD NOT OPEN DIRECTORY\n");
        return 0;
    }

    struct dirent* entry;
    struct stat fileStat;

    while ((entry = readdir(cur_dir)) != NULL) {

        char pathBuffer[MAX_PATH_LEN];
        snprintf(pathBuffer, sizeof(pathBuffer), "%s/%s", directory, entry->d_name);
        stat(pathBuffer, &fileStat);

        
        if (S_ISREG(fileStat.st_mode)) {
            sharedData->fileCount++;

            char* filetype = (char *) malloc(FILE_TYPE);
            filetype = getTypeOfFile(entry->d_name);
            int index = findFileType(sharedData->types, filetype);
            if (index != -1) sharedData->types[index].num ++;
            else {
                struct FileTypes t;
                t.num = 1;
                strcpy(t.type, filetype);
                sharedData->types[sharedData->filetype_num] = t;
                sharedData->filetype_num ++;
            }


            if (fileStat.st_size > sharedData->max_size) {
                sharedData->max_size = fileStat.st_size;
                strcpy(sharedData->largestFile, entry->d_name);
            }
            else if (fileStat.st_size < sharedData->min_size) {
                sharedData->min_size = fileStat.st_size;
                strcpy(sharedData->smallestFile, entry->d_name);
            }

            printf("\nFile: %s, size: %ld, PID: %d\n\n", entry->d_name, fileStat.st_size, getpid());
        }
        else if (S_ISDIR(fileStat.st_mode) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) { //if identify directory create child process to traverse it 

            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            
            if (pid == 0) {  // Child process
                traverseWithThread(pathBuffer);
                exit(EXIT_SUCCESS);

            }
            else {
                wait(NULL);
            }

        }
    }
    closedir(cur_dir);
    printFileInfo();
    return 1;
}


void *traverseWithThread(char *directory) {
    sem_wait(&semaphore);
    printf("\nCREATE NEW PROCECE(PID = %d) FOR TRAVERS DIRECTORY %s\n", getpid(), directory);

    DIR *cur_dir = opendir(directory);
    if (!cur_dir) {
        printf("COULD NOT OPEN DIRECTORY\n");
        sem_post(&semaphore);
        return NULL;
    }

    struct dirent* entry;
    struct stat fileStat;

    while ((entry = readdir(cur_dir)) != NULL) {

        char pathBuffer[MAX_PATH_LEN];
        snprintf(pathBuffer, sizeof(pathBuffer), "%s/%s", directory, entry->d_name);
        stat(pathBuffer, &fileStat);

        
        if (S_ISREG(fileStat.st_mode)) {

            sharedData->fileCount++;


            char* filetype = (char *) malloc(FILE_TYPE);
            filetype = getTypeOfFile(entry->d_name);
            int index = findFileType(sharedData->types, filetype);
            if (index != -1) sharedData->types[index].num ++;
            else {
                struct FileTypes t;
                t.num = 1;
                strcpy(t.type, filetype);
                sharedData->types[sharedData->filetype_num] = t;
                sharedData->filetype_num ++;
            }


            if (fileStat.st_size > sharedData->max_size) {
                sharedData->max_size = fileStat.st_size;
                strcpy(sharedData->largestFile, entry->d_name);
            }
            else if (fileStat.st_size < sharedData->min_size) {
                sharedData->min_size = fileStat.st_size;
                strcpy(sharedData->smallestFile, entry->d_name);
            }

            printf("\nFile: %s, size: %ld, PID: %d\n\n", entry->d_name, fileStat.st_size, getpid());

        }
        else if (S_ISDIR(fileStat.st_mode) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {  //if identify directory create thraed to traverse it 
            
            // Create a structure to hold arguments
            struct ThreadArgs *args = (struct ThreadArgs *)malloc(sizeof(struct ThreadArgs));
            strcpy(args->directory, pathBuffer);

            // Create a thread for the directory
            pthread_t tid;
            pthread_create(&tid, NULL, &threadFunc, (void *)args);
            pthread_join(tid, NULL);  

        }
    }
    closedir(cur_dir);
    sem_post(&semaphore);
    return NULL;

}


void *threadFunc(void *arg) {

    struct ThreadArgs *args = (struct ThreadArgs *)arg;
    printf("\nCREATE NEW THREAD(TID = %ld) FOR TRAVERS DIRECTORY %s\n", pthread_self(), args->directory);

    // Free the allocated memory for the argument structure
    free(arg);

    DIR *cur_dir = opendir(args->directory);
    if (!cur_dir) {
        printf("COULD NOT OPEN DIRECTORY\n");
        return NULL;
    }

    struct dirent *entry;
    struct stat fileStat;

    while ((entry = readdir(cur_dir)) != NULL) {
        char pathBuffer[MAX_PATH_LEN];
        snprintf(pathBuffer, sizeof(pathBuffer), "%s/%s", args->directory, entry->d_name);
        stat(pathBuffer, &fileStat);

        if (S_ISREG(fileStat.st_mode)) {
                pthread_mutex_lock(&lock);

            sharedData->fileCount++;
            char *filetype = getTypeOfFile(entry->d_name);
            int index = findFileType(sharedData->types, filetype);
            if (index != -1)
                sharedData->types[index].num++;
            else {
                struct FileTypes t;
                t.num = 1;
                strcpy(t.type, filetype);
                sharedData->types[sharedData->filetype_num] = t;
                sharedData->filetype_num++;
            }

            if (fileStat.st_size > sharedData->max_size) {
                sharedData->max_size = fileStat.st_size;
                strcpy(sharedData->largestFile, entry->d_name);
            } else if (fileStat.st_size < sharedData->min_size) {
                sharedData->min_size = fileStat.st_size;
                strcpy(sharedData->smallestFile, entry->d_name);
            }
            pthread_mutex_unlock(&lock);


            printf("\nFile: %s, size: %ld, PID: %d\n\n", entry->d_name, fileStat.st_size, getpid());
        } else if (S_ISDIR(fileStat.st_mode) && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            // Create a structure to hold arguments
            struct ThreadArgs *argss = (struct ThreadArgs *)malloc(sizeof(struct ThreadArgs));
            strcpy(argss->directory, pathBuffer);

            // Create a thread for the directory
            pthread_t tid;
            pthread_create(&tid, NULL, &threadFunc, (void *)argss);
            pthread_join(tid, NULL);  
        }
    }
    closedir(cur_dir);
    return NULL;
}


int findFileType(struct FileTypes filetypes[], char t[]) {
    for (int i = 0; i < MAX_FILE_TYPES; i ++) {
        if (strcmp(filetypes[i].type, t) == 0) return i;
    }
    return -1;
}

char* getTypeOfFile(char name[]) {
    int size = strlen(name);
    char *t = (char *) malloc(FILE_TYPE);
    for (int i = 0; i < size; i ++) {
        if (name[i] == '.') {
            for (int j = i+1; j< size; j ++) {
                t[j-i-1] = name[j];
            }
            return t;
        }
    }
    return strdup(name);

}

void printFileInfo() {
    printf("FileInfo print:\nnumber of total file = %d\nlargestFile = %s (size = %d)\nsmallestFile = %s (size = %d)\n\n", sharedData->fileCount, sharedData->largestFile, sharedData->max_size, sharedData->smallestFile, sharedData->min_size);
    for (int i = 0; i < MAX_FILE_TYPES; i ++) {
        printf("Type : %s,\tnum = %d\n", sharedData->types[i].type, sharedData->types[i].num);
    }
}