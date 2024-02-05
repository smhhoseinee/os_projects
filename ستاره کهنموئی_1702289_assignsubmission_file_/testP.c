#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <stdio.h>
#include <GLUT/glut.h>
#include <OpenGL/gl.h>




#define MAX_FILES 1000
#define MAX_FILE_PATH_LENGTH 256

//struct to hold file information
struct FileInfo {
    int numFiles;
    char fileTypes[MAX_FILES][MAX_FILE_PATH_LENGTH];
    char largestFile[MAX_FILE_PATH_LENGTH];
    char smallestFile[MAX_FILE_PATH_LENGTH];
    long largestFileSize;
    long smallestFileSize;
    long finalSize;
};

//SHM: shared memory key
#define SHM_KEY 1234

//shared memory segment
int shmid;
struct FileInfo *sharedData;

//mutex lock
pthread_mutex_t lock;

//semaphore
sem_t semaphore;

// graphics
#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 400
//button
// typedef struct {
//     float x, y, width, height;
//     const char* text;
//     int clicked;
//     const char* message;
// } Button;

// Button buttons[3] = {
//     {100, 200, 100, 50, "Button 1", 0, },
//     {250, 200, 100, 50, "Button 2", 0, "Button 2 Clicked!"},
//     {400, 200, 100, 50, "Button 3", 0, "Button 3 Clicked!"}
// };
// void drawText(float x, float y, const char* text) {
//     glRasterPos2f(x, y);
//     for (int i = 0; text[i] != '\0'; ++i) {
//         glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, text[i]);
//     }
// }

// void drawButtons() {
//     glClear(GL_COLOR_BUFFER_BIT);
//     glLoadIdentity();
   
//     // Draw buttons
//     for (int i = 0; i < 3; ++i) {
//         glColor3f(0.5f, 0.5f, 0.5f);
//         glBegin(GL_QUADS);
//         glVertex2f(buttons[i].x, buttons[i].y);
//         glVertex2f(buttons[i].x + buttons[i].width, buttons[i].y);
//         glVertex2f(buttons[i].x + buttons[i].width, buttons[i].y + buttons[i].height);
//         glVertex2f(buttons[i].x, buttons[i].y + buttons[i].height);
//         glEnd();
       
//         glColor3f(1.0f, 1.0f, 1.0f);
//         drawText(buttons[i].x + 10, buttons[i].y + 20, buttons[i].text);
//     }

//     // Display information
//     glColor3f(1.0f, 0.0f, 0.0f);
//     drawText(50, 50, sharedData->largestFile);
//     drawText(50, 100, sharedData->smallestFile);
//     char totalSizeText[100];
//     sprintf(totalSizeText, "Total Size: %ld bytes", sharedData->finalSize);
//     drawText(50, 150, totalSizeText);
   
//     glFlush();
// }

// void mouseClick(int button, int state, int x, int y) {
//     if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
//         for (int i = 0; i < 3; ++i) {
//             if (x >= buttons[i].x && x <= buttons[i].x + buttons[i].width &&
//                 y >= buttons[i].y && y <= buttons[i].y + buttons[i].height) {
//                 printf("%s\n", buttons[i].message);
//             }
//         }
//     }
// }

// void initializeGraphics(int argc, char** argv) {
//     glutInit(&argc, argv);
//     glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
//     glutCreateWindow("File Information");
//     gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
//     glutDisplayFunc(drawButtons);
//     glutMouseFunc(mouseClick);
//     glutMainLoop();
// }


//functions:
void *traverseDirectoryWithProcesses(void *path); //for main directory
void *traverseDirectoryWithThreads(void *path); 
//functions for the graphics
// void drawButtons() {
//     //glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
//     glClear(GL_COLOR_BUFFER_BIT);

//     for (int i = 0; i < 3; ++i) {
//         glColor3f(0.5f, 0.5f, 0.5f); // Gray color for button
//         glBegin(GL_QUADS);
//             glVertex2f(buttons[i].x, buttons[i].y);
//             glVertex2f(buttons[i].x + buttons[i].width, buttons[i].y);
//             glVertex2f(buttons[i].x + buttons[i].width, buttons[i].y + buttons[i].height);
//             glVertex2f(buttons[i].x, buttons[i].y + buttons[i].height);
//         glEnd();

//         drawText(buttons[i].text, buttons[i].x + 10, buttons[i].y + 20);
//     }

//     for (int i = 0; i < 3; ++i) {
//         if (buttons[i].clicked) {
//             glColor3f(0.0f, 0.0f, 0.0f); // Black text color
//             drawText(buttons[i].message, 10, 30);
//         }
//     }

//     glFlush();
// }
// void mouseClick(int button, int state, int x, int y) {
//     if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
//         y = WINDOW_HEIGHT - y; // Invert y-coordinate to match OpenGL space

//         for (int i = 0; i < 3; ++i) {
//             if (x >= buttons[i].x && x <= buttons[i].x + buttons[i].width &&
//                 y >= buttons[i].y && y <= buttons[i].y + buttons[i].height) {
//                 buttons[i].clicked = 1;
//                 printf("%s clicked!\n", buttons[i].text);
//             } else {
//                 buttons[i].clicked = 0;
//             }
//         }
//         glutPostRedisplay();
//     }
// }

// void display() {
//     drawButtons();
// }

int main(int argc, char *argv[]) {
    // if (argc != 2) {
    //     printf("Usage: %s <directory_path>\n", argv[0]);
    //     exit(EXIT_FAILURE);
    // }
    //printf("Enter Directory:")
    char *mainDirectory = "/Users/mac/Desktop/Codes/c/ThreadTest"; //argv[1];
    printf("Main process started for folder: %s\n", mainDirectory);

    //initialize mutex
    pthread_mutex_init(&lock, NULL);

    //shared memory segment creation
    shmid = shmget(SHM_KEY, sizeof(struct FileInfo), IPC_CREAT | 0666);

    // if (shmid == -1) {
    //     perror("shmget failed");
    //     exit(EXIT_FAILURE);
    // }

    //attach shared memory
    sharedData = (struct FileInfo *)shmat(shmid, NULL, 0);

    // if (sharedData == (struct FileInfo *)(-1)) {
    //     perror("shmat failed");
    //     exit(EXIT_FAILURE);
    // }

    //initialize shared data
    sharedData->numFiles = 0;
    sharedData->largestFileSize = 0;
    sharedData->smallestFileSize = __INT_MAX__;

    //directory traversal with processes
    traverseDirectoryWithProcesses((void *)mainDirectory);

    //print results
    printf("Total number of files: %d\n", sharedData->numFiles);
    printf("File types:\n");
    for (int i = 0; i < sharedData->numFiles; ++i) {
        printf("%s\n", sharedData->fileTypes[i]);
    }
    printf("Address of the largest file: %s, Size: %ld\n", sharedData->largestFile, sharedData->largestFileSize);
    printf("Address of the smallest file: %s, Size: %ld\n", sharedData->smallestFile, sharedData->smallestFileSize);
    printf("Final size of the main folder: %ld bytes\n", sharedData->finalSize);

    //create the buttons
    
    // buttons[] = {100, 200, 100, 50, "Button 1", 0, "Address of the largest file: %s, Size: %ld\n", sharedData->largestFile, sharedData->largestFileSize},
    // {250, 200, 100, 50, "Button 2", 0, "Address of the smallest file: %s, Size: %ld\n", sharedData->smallestFile, sharedData->smallestFileSize},
    // {400, 200, 100, 50, "Button 3", 0, "Final size of the main folder: %ld bytes\n", sharedData->finalSize}

    //strcpy(buttons[0].message,strcat("Address of the largest file: %s, Size: %ld\n",sharedData->largestFile));// sharedData->largestFileSize);
    //strcpy(buttons[1].message ,"Address of the smallest file: %s, Size: %ld\n" sharedData->smallestFile, sharedData->smallestFileSize);
    //strcpy(buttons[2].message ,"Final size of the main folder: %ld bytes\n", sharedData->finalSize);
    //detach and remove shared memory
    shmdt(sharedData);
    shmctl(shmid, IPC_RMID, NULL);

    //destroy mutex
    pthread_mutex_destroy(&lock);

    //display graphics
    // glutInit(&argc, argv);
    // glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    // glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    // glutCreateWindow("OpenGL Buttons Example");

    // glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // Set clear color to white
    // glMatrixMode(GL_PROJECTION);
    // glLoadIdentity();
    // gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);

    // glutDisplayFunc(display);
    // glutMouseFunc(mouseClick);

    // glutMainLoop();
    //initializeGraphics(argc, argv);

    return 0;
}

void *traverseDirectoryWithProcesses(void *path) { 
    sem_init(&semaphore, 0, 1); //initialie semaphore

    char *currentPath = (char *)path;

    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(currentPath))) {
        perror("opendir error");
        pthread_exit(NULL);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG){
    
            //fnmber of files++
            sharedData->numFiles++;

            //file type
            char fileType[20];
            strcpy(fileType, entry->d_name);
            strcpy(sharedData->fileTypes[sharedData->numFiles - 1], entry->d_name);

            //file size
            char filePath[256];
            snprintf(filePath, sizeof(filePath), "%s/%s", currentPath, entry->d_name);
            FILE *file = fopen(filePath, "r");
            fseek(file, 0L, SEEK_END);
            long fileSize = ftell(file);
            fseek(file, 0L, SEEK_SET);
            fclose(file);

            if (fileSize > sharedData->largestFileSize) {
                sharedData->largestFileSize = fileSize;
                strcpy(sharedData->largestFile, filePath);
            }

            if (fileSize < sharedData->smallestFileSize) {
                sharedData->smallestFileSize = fileSize;
                strcpy(sharedData->smallestFile, filePath);
            }

        printf("File(in the main process): %s, with size: %ld\n", filePath, fileSize);

        }
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char subFolderPath[256];
            snprintf(subFolderPath, sizeof(subFolderPath), "%s/%s", currentPath, entry->d_name);

            pid_t pid = fork();
            if (pid < 0) {
                perror("fork error");
                closedir(dir);
                //pthread_exit(NULL);
            } else if (pid == 0) { //child process
                printf("Child process created for folder: %s with pid:%ld\n", subFolderPath, getpid());
                traverseDirectoryWithThreads((void *)subFolderPath); // Second traversal with threads
                closedir(dir);
                //pthread_exit(NULL);
            } else { //parent process
                wait(NULL); //wait for child process to finish before continuing
            }
        }
    }

    closedir(dir);
    //exitProcess(NULL);
}

void *traverseDirectoryWithThreads(void *path) {
    sem_wait(&semaphore); //semaphore
    char *currentPath = (char *)path;

    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(currentPath))) {
        perror("opendir error");
        pthread_exit(NULL);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            pthread_mutex_lock(&lock); //lock critical section

            //number of files ++
            sharedData->numFiles++;

            //file type
            char fileType[20];
            strcpy(fileType, entry->d_name);
            strcpy(sharedData->fileTypes[sharedData->numFiles - 1], entry->d_name);

            //file size
            char filePath[256];
            snprintf(filePath, sizeof(filePath), "%s/%s", currentPath, entry->d_name);
            FILE *file = fopen(filePath, "r");
            fseek(file, 0L, SEEK_END);
            long fileSize = ftell(file);
            fseek(file, 0L, SEEK_SET);
            fclose(file);

            if (fileSize > sharedData->largestFileSize) {
                sharedData->largestFileSize = fileSize;
                strcpy(sharedData->largestFile, filePath);
            }

            if (fileSize < sharedData->smallestFileSize) {
                sharedData->smallestFileSize = fileSize;
                strcpy(sharedData->smallestFile, filePath);
            }

            sharedData->finalSize += fileSize;

            pthread_mutex_unlock(&lock); //unlock critical section
            printf("File: %s, with size: %ld\n", filePath, fileSize);

            
        } else if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char subFolderPath[256];
            snprintf(subFolderPath, sizeof(subFolderPath), "%s/%s", currentPath, entry->d_name);

            pthread_t thread;
            printf("New thread created for folder: %s with id:%ld\n", subFolderPath, pthread_self());
            pthread_create(&thread, NULL, traverseDirectoryWithThreads, (void *)subFolderPath);
            pthread_join(thread, NULL);
        }
    }
    sem_post(&semaphore); //semaphore

    closedir(dir);
    pthread_exit(NULL);

}

