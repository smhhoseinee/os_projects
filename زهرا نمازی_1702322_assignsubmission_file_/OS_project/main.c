#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h> 
#include <string.h> 
#include <stdlib.h>
#include <math.h>
#include <sys/ipc.h>
#include <sys/shm.h>


pthread_mutex_t printMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t total_files_Mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t types_Mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t min_max_Mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t total_size_Mutex = PTHREAD_MUTEX_INITIALIZER;


typedef struct
{
   int count;
} DOTDATA;


typedef struct
{
    int depth; 
    char path[500];
}t_arg;


typedef struct {
    char type[10];
    int number;
}FileType;

typedef struct 
{
    int total_files;
    int numTypes;
    long long max_size;
    long long min_size;
    long long total_size;
    char max_directory[500];
    char min_directory[500];
    FileType fileTypes[500];
}result;

DOTDATA dc;

result* r;
char *get_type(const char* name){
    char *splitName = strrchr(name, '.');
    if(!splitName || splitName == name)
        return "";
    return splitName + 1;
}
void addType(FileType* fType, char* type, int* len){
    for(int i = 0; i  < (*len); i++){
        if(strcmp(fType[i].type, type) == 0){
            fType[i].number++;
            //printf("test\n");
            return;
        }
    }
    FileType ft;
    ft.number = 1;
    strcpy(ft.type, type);
    fType[*len] = ft;
    (*len)++;
}
int isDir(const char *path) {
    struct stat st;
    
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    } else {
        // Handle error, e.g., print or log an error message
        perror("stat");
        return -1;  // Indicate failure
    }
}


void *threadFun(void* args){
    
    t_arg *a = (t_arg*)args;
    //pthread_mutex_lock(&printMutex);
    //printf("\nthread %s\n", a->path);
    //pthread_mutex_unlock(&printMutex);
    directory_task(a->path, a->depth);
    pthread_exit(NULL);
}


void make_path(char parent[], char child[], char *path){
    strcpy(path, parent);
    strcat(path, "/");
    strcat(path, child);
}


void first_task(char dir_address[]){
    struct dirent *de;
    struct dirent* files[500] = {NULL};
    struct dirent* directories[500] = {NULL};
    int files_index = 0;
    int dir_index = 0;
	DIR *dr = opendir(dir_address); 
    
    
    pid_t pids[200]; //number of process in main directory
    int pids_index = 0;


	if (dr == NULL) // opendir returns NULL if couldn't open directory 
	{ 
		printf("Could not open current directory" );
	} 

	// Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html 
	// for readdir() 
	while ((de = readdir(dr)) != NULL) {
        if(!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")){
           continue;
        }
        char path[500];
        make_path(dir_address, de->d_name, path);
        if(isDir(path) == 1) {
            directories[dir_index] = de;
            dir_index++;
        } else {
            files[files_index] = de;
            //min max
            struct stat file;
            stat(path, &file);
            long long curSize = (long long)file.st_size;
            //printf("curr%lli\n", curSize);
            pthread_mutex_lock(&min_max_Mutex);
            if((*r).max_size < curSize){
                (*r).max_size = curSize;
                strcpy((*r).max_directory, path);
            }
            if((*r).min_size > curSize){
                (*r).min_size = curSize;
                strcpy((*r).min_directory, path);
            }
            pthread_mutex_unlock(&min_max_Mutex);
            //min max
            //printf("max%lli---min%lli--->>curr%lli--->>>in first task \n", (*r).max_size, (*r).min_size, curSize);
            //dc.count++;
            pthread_mutex_lock(&total_files_Mutex);
            (*r).total_files++;
            pthread_mutex_unlock(&total_files_Mutex);
            pthread_mutex_lock(&total_size_Mutex);
            (*r).total_size += curSize;
            pthread_mutex_unlock(&total_size_Mutex);
            pthread_mutex_lock(&total_files_Mutex);
            addType((*r).fileTypes,get_type(path), &(*r).numTypes);
            pthread_mutex_unlock(&total_files_Mutex);
            
            files_index++;
        }
        char empty[] = "";
        strcpy(path, empty);
    }
    //printf("-----------------\n\n");

    for (int i = 0; i < dir_index; i++) {
        // pthread_mutex_lock(&printMutex);
        // printf("%s\n", directories[i]->d_name);
        // pthread_mutex_unlock(&printMutex);
        pid_t pid = fork(); // Create a new process
        if (pid < 0) {
            // Fork failed
            fprintf(stderr, "Fork failed\n");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // This code is executed by the child process

            //printf("Child process %d is running\n", i);
            char path[500];
            make_path(dir_address, directories[i]->d_name ,path);
            directory_task(path, 1);

            exit(EXIT_SUCCESS); // Terminate the child process
        }else{
            int returnStatus;    
            waitpid(pid, &returnStatus, 0);
        }
        
    }

    // This code is executed by the parent process
    // Parent process may perform some other tasks or wait for child processes

    for (int i = 0; i < dir_index; i++) {
        wait(NULL); // Wait for each child process to finish
    }
    // pthread_mutex_lock(&printMutex);
    // for (int i = 0; i<files_index; i++) {
    //     printf("%s\n", files[i]->d_name);
    // }
    // pthread_mutex_unlock(&printMutex);
    //printf("All child processes have terminated\n");

	closedir(dr);	 
	return; 
}

void directory_task(char dir_address[], int depth){
    struct dirent *de;
    struct dirent* files[500] = {NULL};
    struct dirent* directories[500] = {NULL};
    int files_index = 0;
    int dir_index = 0;
	DIR *dr = opendir(dir_address); 
    pthread_t threads[500];
    int thread_args[500];

	if (dr == NULL) // opendir returns NULL if couldn't open directory 
	{ 
		printf("Could not open current directory" );
	} 

	
	while ((de = readdir(dr)) != NULL) {
        if(!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")){
           continue;
        }
        char path[500];
        make_path(dir_address, de->d_name, path);
        if(isDir(path) == 1) {

            // pthread_mutex_lock(&printMutex);
            // for(int i=0;i<depth; i++){
            //     printf("\t");
            // }
            // printf("%s\n", de->d_name);
            // pthread_mutex_unlock(&printMutex);
            directories[dir_index] = de;
            thread_args[dir_index] = dir_index;
            t_arg args;
            strcpy(args.path, path);
            args.depth = depth + 1;
            pthread_create(&threads[dir_index], NULL, threadFun, &args);
            pthread_join(threads[dir_index], NULL);
            dir_index++;
        } else {
            files[files_index] = de;
            //min max
            struct stat file;
            stat(path, &file);
            long long curSize = (long long)file.st_size;
            //printf("curr is %lli\n", curSize);
            pthread_mutex_lock(&min_max_Mutex);
            if((*r).max_size < curSize){
                (*r).max_size = curSize;
                strcpy((*r).max_directory, path);
            }
            if((*r).min_size > curSize){
                (*r).min_size = curSize;
                strcpy((*r).min_directory, path);
            }
            pthread_mutex_unlock(&min_max_Mutex);
            // //min max
            //printf("max%lli---min%lli--->>curr%lli--->>>in directory task \n", (*r).max_size, (*r).min_size, curSize);
            dc.count++;
            //printf("count%d\n", dc.count);
            pthread_mutex_lock(&total_files_Mutex);
            (*r).total_files++;
            pthread_mutex_unlock(&total_files_Mutex);
            pthread_mutex_lock(&total_size_Mutex);
            (*r).total_size += curSize;
            pthread_mutex_unlock(&total_size_Mutex);
            pthread_mutex_lock(&total_files_Mutex);
            addType((*r).fileTypes,get_type(path), &(*r).numTypes);
            pthread_mutex_unlock(&total_files_Mutex);
            files_index++;
        }
        char empty[] = "";
        strcpy(path, empty);
    }
 
    // pthread_mutex_lock(&printMutex);
    // printf("\n-----------------\n\n");
    // for (int i = 0; i<files_index; i++) {
    //     for(int j=0;j<depth; j++){
    //         printf("\t");
    //     }
    //     printf("%s\n", files[i]->d_name);
    // }
    // printf("max size: %lli, address of max file: %s\n", max_size, max_directory);
    // printf("min size: %lli, address of min file: %s\n", min_size, min_directory); 
    ///pthread_mutex_unlock(&printMutex);
    //pthread_exit(NULL);
	closedir(dr);	 
	return;
}

int main(void) 
{ 
    char dir_address[500];
    printf("enter");
    scanf("%s", dir_address);

    //key_t key = ftok("/path/to/your/file", 'R');
    int shm_id = shmget(IPC_PRIVATE, sizeof(result), IPC_CREAT | 0666);
    //printf("id of shm:%d ", shm_id);
    r = (result*) shmat(shm_id, NULL, 0);
    (*r).max_size = 0;
    (*r).min_size = INFINITY;
    (*r).numTypes = 0;
    (*r).total_files = 0;
    (*r).total_size = 0;
    //printf("%d---%lli--- ", (*r).max_size, (*r).min_size);
    first_task(dir_address);
    printf("max size: %.3f MB, address of max file: %s\n", ((*r).max_size)/1000000.0, (*r).max_directory);
    printf("min size: %.3f MB, address of min file: %s\n", ((*r).min_size)/1000000.0, (*r).min_directory); 
    printf("total number of files in root directory: %d\n", (*r).total_files);
    printf("total size of files in root directory: %.3f MB\n", ((*r).total_size)/1000000.0);
    printf("file types: %d\n", (*r).numTypes);
    for(int i = 0; i < (*r).numTypes; i++)
        printf("type: .%s  : %d\n", (*r).fileTypes[i].type, (*r).fileTypes[i].number);
    //printf("%d\n", dc.count);
    

    shmdt(r);
    return 0;
} 

