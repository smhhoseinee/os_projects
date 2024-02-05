#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <pthread.h>
#include "utilities.h"
#include "arraylist.h"

//Uncomment the line below to print the monitoring outputs
//#define MONITOR 1;

void initDirectoryTask(struct task *task, char input[]) {
    DIR *dirp;
    struct dirent *direntp;

    unsigned long max = 0;
    unsigned long long min = ULLONG_MAX;

    unsigned long long folderSize = 0;

    dirp = opendir(input);

    if (dirp == NULL) {
        perror("Invalid File Handle.\\n");
    } else {
        for (;;) {
            direntp = readdir(dirp);
            if (direntp == NULL) break;

            if (strcmp(direntp->d_name, ".") == 0 || strcmp(direntp->d_name, "..") == 0 ||
                strcmp(direntp->d_name, ".DS_Store") == 0)
                continue;

            // Directory
            if (direntp->d_type == 4) {
                char filepath[1000];
                sprintf(filepath, "%s%s/", input, direntp->d_name);

                struct iDirectory directory;
                directory.valid = 1;
                strcpy(directory.name, direntp->d_name);
                strcpy(directory.path, filepath);

                appendToDirectoryArray(task, directory);
            }
                // File
            else if (direntp->d_type == 8) {
                char filepath[1000];
                sprintf(filepath, "%s%s", input, direntp->d_name);

                struct iFile file;
                file.valid = 1;
                strcpy(file.name, direntp->d_name);
                strcpy(file.path, filepath);
                strcpy(file.extension, get_extension(direntp->d_name));

                long int fileSize = findSize(filepath);

                file.size = fileSize;
                folderSize = folderSize + fileSize;


                appendToFileArray(task, file);

                task->dirSize = folderSize;

                char *extension = get_extension(direntp->d_name);
                extensionTypes(extension, task);
            }
        }


        for (int i = 0; i < task->filesCount; i++) {
            // Updating the Largest File
            if (task->files[i].size > max) {
                task->maxSize = task->files[i];
                max = task->files[i].size;
            }

            // Updating the Smallest File
            if (task->files[i].size < min) {
                task->minSize = task->files[i];
                min = task->files[i].size;
            }
        }
        closedir(dirp);
    }
}

pthread_mutex_t lock;

void *threadFunction(void *arg) {
    struct thread_arg *argument;
    argument = (struct thread_arg *) arg;

    initDirectoryTask(argument->threadTask, argument->path);

    //-----------------------------------------------------------------------------------------------------------------
    // Monitoring outputs:
    #ifdef MONITOR
        printf("Thread %d-> Analyses Directory : %s\n", pthread_self(), argument->path);
        printf("Thread %d-> Max File is : %s with Size: %lu\n", pthread_self(), argument->threadTask->maxSize.path,
               argument->threadTask->maxSize.size);
        printf("Thread %d-> Min File is : %s with Size: %lu\n", pthread_self(), argument->threadTask->minSize.path,
               argument->threadTask->minSize.size);
    #endif

    //-----------------------------------------------------------------------------------------------------------------


    if (argument->threadTask->directoryCount == 0 && argument->threadTask->filesCount == 0) {
        pthread_exit(NULL);
    };

    pthread_t tid[argument->threadTask->directoryCount];
    struct thread_arg threadArguments[argument->threadTask->directoryCount];
    struct task threadTasks[argument->threadTask->directoryCount];

    for (int i = 0; i < argument->threadTask->directoryCount; ++i) {
        threadArguments[i].path = argument->threadTask->directory[i].path;
        threadArguments[i].task = argument->threadTask;
        threadArguments[i].threadTask = &threadTasks[i];
        pthread_create(&tid[i], NULL, threadFunction, &threadArguments[i]);
    }

    for (int j = 0; j < argument->threadTask->directoryCount; j++) {
        pthread_join(tid[j], NULL);
    }

    pthread_mutex_lock(&lock);
    if (strlen(argument->task->maxSize.name) != 0) {
        if (argument->threadTask->maxSize.size > argument->task->maxSize.size) {
            argument->task->maxSize = argument->threadTask->maxSize;
        }
    } else {
        argument->task->maxSize = argument->threadTask->maxSize;
    }

    if (strlen(argument->task->minSize.name) != 0) {
        if (argument->threadTask->minSize.size < argument->task->minSize.size) {
            argument->task->minSize = argument->threadTask->minSize;
        }
    } else {
        argument->task->minSize = argument->threadTask->minSize;
    }


    for (int j = 0; j < argument->threadTask->extensionsCount; ++j) {
        extensionTypesWithCount(argument->threadTask->extensions[j], argument->task);
    }

    argument->task->dirSize += argument->threadTask->dirSize;

    pthread_mutex_unlock(&lock);

    pthread_exit(NULL);
}


char *printsize(size_t size, char *str, size_t len) {
    static const char *SIZES[] = {"B", "kB", "MB", "GB"};
    size_t div = 0;
    size_t rem = 0;

    while (size >= 1000 && div < (sizeof SIZES / sizeof *SIZES)) {
        rem = (size % 1000);
        div++;
        size /= 1000;
    }

    snprintf(str, len, "%.2f %s", (float) size + (float) rem / 1000.0, SIZES[div]);
    str[len - 1] = '\0';
    return str;
}


int main(int argc, char *argv[]) {

    //Putting the main task in shared memory
    struct task *task = (struct task *) mmap(NULL, sizeof(struct task), PROT_READ | PROT_WRITE,
                                             MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (pthread_mutex_init(&lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }

    if (argc == 2) {

        initDirectoryTask(task, argv[1]);

        for (int i = 0; i < task->directoryCount; ++i) {

            pid_t pid;
            pid = fork();
            if (pid == 0) {
                struct task childTask;
                initDirectoryTask(&childTask, task->directory[i].path);

                if (childTask.directoryCount != 0) {
                    pthread_t tid[childTask.directoryCount];
                    struct thread_arg threadArguments[childTask.directoryCount];
                    struct task threadTasks[childTask.directoryCount];

                    for (int j = 0; j < childTask.directoryCount; j++) {
                        threadArguments[j].path = childTask.directory[j].path;
                        threadArguments[j].task = &childTask;
                        threadArguments[j].threadTask = &threadTasks[j];
                        pthread_create(&tid[j], NULL, threadFunction, (void *) &threadArguments[j]);
                    }
                    for (int j = 0; j < childTask.directoryCount; j++) {
                        pthread_join(tid[j], NULL);
                    }
                }

                if (childTask.filesCount != 0) {

                    //Updating the Largest File
                    if (strlen(childTask.maxSize.name) != 0) {
                        if (childTask.maxSize.size > task->maxSize.size) {
                            task->maxSize = childTask.maxSize;
                        }
                    } else {
                        task->maxSize = childTask.maxSize;
                    }

                    //Updating the Smallest File
                    if (strlen(childTask.minSize.name) != 0) {
                        if (childTask.minSize.size < task->minSize.size) {
                            task->minSize = childTask.minSize;
                        }
                    } else {
                        task->minSize = childTask.minSize;
                    }


                    //Counting the number of extensions
                    for (int j = 0; j < childTask.extensionsCount; ++j) {
                        extensionTypesWithCount(childTask.extensions[j], task);
                    }


                    //Counting the Total Number of Files
                    task->dirSize += childTask.dirSize;
                }
                exit(0);
            } else if (pid > 0) {
//                printf("Parent\n");
            } else {
                printf("Error\n");
            }
        }

        while (wait(NULL) != -1);

        pthread_mutex_destroy(&lock);

        //-----------------------------------------------------------------------------------------------------------------
        //printing the results:
        int numOfFiles = 0;
        char largestSize[20];
        char smallestSize[20];
        char rootSize[20];

        printf("--> Number of each file type:\n");
        for (int i = 0; i < task->extensionsCount; ++i) {
            printf("- .%s: %d\n", task->extensions[i].extension, task->extensions[i].count);
            numOfFiles += task->extensions[i].count;
        }
        printf("--> Total Number of Files: %d\n", numOfFiles);
        printf("\n--> File with the largest size: %s, Size: ", task->maxSize.path);
        printf("%s", printsize(task->maxSize.size, largestSize, sizeof(largestSize)));
        printf("\n--> File with the smallest size: %s, Size: ", task->minSize.path);
        printf("%s", printsize(task->minSize.size, smallestSize, sizeof(smallestSize)));
        printf("\n--> Size of Root Folder: ");
        printf("%s", printsize(task->dirSize, rootSize, sizeof(rootSize)));

        //-----------------------------------------------------------------------------------------------------------------
        // Writing to file:

        FILE *fptr;
        fptr = fopen("/Users/mbina/CLionProjects/OS_NCDU/filename.txt", "w");
        //Total num of files
        fprintf(fptr, "%d\n", numOfFiles);

        //File with the largest size - path
        fprintf(fptr, "%s\n", task->maxSize.path);

        //File with the largest size - size
        fprintf(fptr, "%s\n", printsize(task->maxSize.size, largestSize, sizeof(largestSize)));

        //File with the smallest size - path
        fprintf(fptr, "%s\n", task->minSize.path);

        //File with the smallest size - size
        fprintf(fptr, "%s\n", printsize(task->minSize.size, smallestSize, sizeof(smallestSize)));

        //Size of Root Folder
        fprintf(fptr, "%s\n", printsize(task->dirSize, rootSize, sizeof(rootSize)));

        //Number of each file type
        for (int i = 0; i < task->extensionsCount; ++i) {
            if (i == task->extensionsCount - 1) {
                fprintf(fptr, ".%s x%d ", task->extensions[i].extension, task->extensions[i].count);
            } else {
                fprintf(fptr, ".%s x%d - ", task->extensions[i].extension, task->extensions[i].count);
            }
        }

        fclose(fptr);
        //-----------------------------------------------------------------------------------------------------------------

    } else if (argc > 2) {
        printf("Too many arguments supplied.\n");
    } else {
        printf("One argument expected.\n");
    }
}