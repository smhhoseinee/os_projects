#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h> 
#include <signal.h>
#include <pthread.h> 
#include <stdlib.h>
#include <unistd.h>


pthread_mutex_t printMutex = PTHREAD_MUTEX_INITIALIZER;
char empty[] = "";

struct Node{
    char name[500];
    int isFile; // 1:file 0:dir
    struct Node* rightMostChild; //the greates (first) child is the rightmost child
    struct Node* left;
    struct Node* right;
};

typedef struct
{
    struct Node *node; 
    char path[500];
}t_arg;



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
    // pthread_mutex_lock(&printMutex);
    // //printf("\nthread %s\n", a->path);
    // pthread_mutex_unlock(&printMutex);
    directory_task(a->path, a->node);
    pthread_exit(NULL);
}

void make_path(char parent[], char child[], char *path){
    strcpy(path, parent);
    strcat(path, "/");
    strcat(path, child);
}

void make_node(struct Node* node, char* name, int isFile, struct Node* rightchild, struct Node*right, struct Node* left){
    strcpy((*node).name, name);
    (*node).isFile = isFile;
    (*node).rightMostChild = rightchild;
    (*node).right = right;
    (*node).left = left;
}

void print_tree(struct Node* root, int depth) {
    for(int i=0;i<depth;i++){
        printf("\t");
    }
    printf("%s\n", (*root).name);
    struct Node* child = (*root).rightMostChild;
    while(child != NULL && (*child).isFile == 0){
        print_tree(child, depth + 1);
        child = (*child).left;
    }
    while(child != NULL && (*child).isFile == 1){
        for(int i=0;i<depth;i++){
        printf("\t");
        }
        printf("%s\n", (*child).name);
        child = (*child).left;
    }
}

struct Node* first_task(char dir_address[]){
    struct dirent *de;
    struct dirent* files[500] = {NULL};
    struct dirent* directories[500] = {NULL};
    int files_index = 0;
    int dir_index = 0;
	DIR *dr = opendir(dir_address);
    struct Node rootDirectory;

    pid_t pids[200]; //number of process in main directory
    int pids_index = 0;


	if (dr == NULL) // opendir returns NULL if couldn't open directory 
	{ 
		printf("Could not open current directory" );
	} 
    make_node(&rootDirectory, dir_address, 0, NULL, NULL, NULL);
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
            files_index++;
        }
        strcpy(path, empty);
    }


    struct Node* pre_childNode;
    for (int i = 0; i < dir_index; i++) {
        struct Node curr_childNode;
        make_node(&curr_childNode, directories[i]->d_name, 0, NULL, NULL, NULL);
        if(i==0){
            rootDirectory.rightMostChild = &curr_childNode;
        }
        else {
            (*pre_childNode).left = &curr_childNode;
            curr_childNode.right = pre_childNode;
        }
        pid_t pid = fork(); // Create a new process

        if (pid < 0) { // Fork failed
            fprintf(stderr, "Fork failed\n");
            exit(EXIT_FAILURE);
        } else if (pid == 0) { // This code is executed by the child process
            printf("Child process %d is running\n", i); 
            char path[500];
            make_path(dir_address, directories[i]->d_name, path);
            directory_task(path, &curr_childNode);

            // Perform a different task for each child process
            // For example, you can use the value of 'i' to determine the task

            exit(EXIT_SUCCESS); // Terminate the child process
        }
        pre_childNode = &curr_childNode;
    }

    // This code is executed by the parent process
    // Parent process may perform some other tasks or wait for child processes

    for (int i = 0; i < dir_index; i++) {
        wait(NULL); // Wait for each child process to finish
    }
    //pthread_mutex_lock(&printMutex);
    for (int i = 0; i<files_index; i++) {
        struct Node curr_childNode;
        make_node(&curr_childNode, files[i]->d_name, 1, NULL, NULL, NULL);
        if(i==0 && dir_index == 0){
            rootDirectory.rightMostChild = &curr_childNode;
        }
        else {
            (*pre_childNode).left = &curr_childNode;
            curr_childNode.right = pre_childNode;
        }
        pre_childNode = &curr_childNode;
        //printf("%s\n", files[i]->d_name);
    }
    //pthread_mutex_unlock(&printMutex);
    //printf("All child processes have terminated\n");


	closedir(dr);	 
	return &rootDirectory; 
}


void directory_task(char dir_address[], struct Node* root){
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

	struct Node* pre_childNode;
	while ((de = readdir(dr)) != NULL) {
        if(!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")){
           continue;
        }
        char path[500];
        make_path(dir_address, de->d_name, path);
        if(isDir(path) == 1) {
            directories[dir_index] = de;
            thread_args[dir_index] = dir_index;
            //char *args = path;
            struct Node curr_cildNode;
            make_node(&curr_cildNode, de->d_name, 0, NULL, NULL, NULL);
            if(dir_index==0){
                (*root).rightMostChild = &curr_cildNode;
            }
            else {
                (*pre_childNode).left = &curr_cildNode;
                curr_cildNode.right = pre_childNode;
            }
            t_arg args;
            strcpy(args.path, path);
            args.node = &curr_cildNode;
            pthread_create(&threads[dir_index], NULL, threadFun, &args);
            pthread_join(threads[dir_index], NULL);
            dir_index++;
            //free(args);
            pre_childNode = &curr_cildNode;
        } else {
            files[files_index] = de;
            files_index++;
        }
        char empty[] = "";
        strcpy(path, empty);
    }
    for (int i = 0; i<files_index; i++) {
        struct Node curr_childNode;
        make_node(&curr_childNode, files[files_index], 1, NULL, NULL, NULL);
        if(i==0 && dir_index==0){
            (*root).rightMostChild = &curr_childNode;
        }
        else {
            (*pre_childNode).left = &curr_childNode;
            curr_childNode.right = pre_childNode;
        }
        pre_childNode = &curr_childNode;
    }
    // pthread_mutex_unlock(&printMutex);
    
	closedir(dr);	 
	return;
}


int main(void) 
{ 
    char dir_address[500];
    printf("enter");
    scanf("%s", dir_address);
    struct Node* root = first_task(dir_address);
    //print_tree(root, 0);
    return 0;
} 

