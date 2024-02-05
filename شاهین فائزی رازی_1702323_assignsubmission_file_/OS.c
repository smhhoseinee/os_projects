#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <syscall.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <limits.h>


struct arg {

    char* path;

    int  id;

};

pthread_t threads[30000];

int threadCount = 0;

pthread_mutex_t mutexQueue;

pthread_mutex_t mutexCount;

int FileCount = 0;

int FilePipeCount = 0;

int FilePipes[10000 + 1][2];

int TxtCount = 0;

int TxtPipeCount = 0;

int TxtPipes[10000 + 1][2];

int PdfCount = 0;

int PdfPipeCount = 0;

int PdfPipes[10000 + 1][2];

int JpgCount = 0;

int JpgPipeCount = 0;

int JpgPipes[10000 + 1][2];

int PngCount = 0;

int PngPipeCount = 0;

int PngPipes[10000 + 1][2];

long int MaxSize = -1;

int MaxSizePipeCount = 0;

int MaxSizePipes[10000 + 1][2];

long int MinSize = LONG_MAX;

int MinSizePipeCount = 0;

int MinSizePipes[10000 + 1][2];

long long RSize = 0;

int RSizePipeCount = 0;

int RSizePipes[10000 + 1][2];



const char* get_filename_ext(const char* filename) {

    const char* dot = strrchr(filename, '.');

    if (!dot || dot == filename) return "";

    return dot;
}

long get_file_size(char* filename) {
    struct stat file_status;
    if (stat(filename, &file_status) < 0) {
        return -1;
    }

    return (file_status.st_size); //kb ->return (file_status.st_size);
}


void* search(void* arguments);



void submitThread(struct arg* argsPass) {

    pthread_mutex_lock(&mutexQueue);

    pthread_t* thread = (pthread_t*)malloc(sizeof(pthread_t));

    pthread_attr_t attr;

    pthread_attr_init(&attr);

    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_create(thread, NULL, search, (void*)argsPass);

    threads[threadCount] = *thread;

    threadCount++;

    pthread_mutex_unlock(&mutexQueue);


}


void* search(void* arguments) {

    struct arg* args = (struct arg*)malloc(sizeof(struct arg));

    args = (struct arg*)arguments;

    char* path = args->path;

    int id = args->id;

    DIR* dirFile;

    if (!(dirFile = opendir(path))) {
        return NULL;
    }

    struct dirent* hFile;

    while ((hFile = readdir(dirFile)) != NULL)
    {

        if (strcmp(hFile->d_name, ".") == 0 || strcmp(hFile->d_name, "..") == 0)
            continue;

        char filepath[strlen(path) + 1 + strlen(hFile->d_name) + 1];

        snprintf(filepath, sizeof filepath, "%s/%s", path, hFile->d_name);

        if (hFile->d_type == DT_REG) {


            printf("file : %s\n", hFile->d_name);

            pthread_mutex_lock(&mutexCount);

            //max

            long int tempMax;

            int tempN;

            read(MaxSizePipes[MaxSizePipeCount][0], &tempMax, sizeof(long int));

            read(MaxSizePipes[MaxSizePipeCount][0], &tempN, sizeof(int));

            char* tempPathh = (char*)malloc(tempN + 1);

            read(MaxSizePipes[MaxSizePipeCount][0], tempPathh, tempN * sizeof(char));

            long int tempM;

            char* temp = (char*)malloc(strlen(filepath) + 1);

            strcpy(temp, filepath);

            tempM = get_file_size(temp);

            if (tempM > tempMax) {

                MaxSize = tempM;

                int tempNN = strlen(temp) + 1;

                write(MaxSizePipes[MaxSizePipeCount][1], &MaxSize, sizeof(long int));

                write(MaxSizePipes[MaxSizePipeCount][1], &tempNN, sizeof(int));

                write(MaxSizePipes[MaxSizePipeCount][1], temp, tempNN * sizeof(char));

            }
            else {

                write(MaxSizePipes[MaxSizePipeCount][1], &tempMax, sizeof(long int));

                write(MaxSizePipes[MaxSizePipeCount][1], &tempN, sizeof(int));

                write(MaxSizePipes[MaxSizePipeCount][1], tempPathh, tempN * sizeof(char));


            }

            //min

            long int tempMin;

            int tempN_;

            read(MinSizePipes[MinSizePipeCount][0], &tempMin, sizeof(long int));

            read(MinSizePipes[MinSizePipeCount][0], &tempN_, sizeof(int));

            char* tempPathh_ = (char*)malloc(tempN_ + 1);

            read(MinSizePipes[MinSizePipeCount][0], tempPathh_, tempN_ * sizeof(char));

            long int tempM_;

            char* temp_ = (char*)malloc(strlen(filepath) + 1);

            strcpy(temp_, filepath);

            tempM_ = get_file_size(temp_);

            if (tempM_ < tempMin) {

                MinSize = tempM_;

                int tempNN_ = strlen(temp_) + 1;

                write(MinSizePipes[MinSizePipeCount][1], &MinSize, sizeof(long int));

                write(MinSizePipes[MinSizePipeCount][1], &tempNN_, sizeof(int));

                write(MinSizePipes[MinSizePipeCount][1], temp_, tempNN_ * sizeof(char));

            }
            else {

                write(MinSizePipes[MinSizePipeCount][1], &tempMin, sizeof(long int));

                write(MinSizePipes[MinSizePipeCount][1], &tempN_, sizeof(int));

                write(MinSizePipes[MinSizePipeCount][1], tempPathh_, tempN_ * sizeof(char));


            }



            read(FilePipes[FilePipeCount][0], &FileCount, sizeof(int));

            FileCount++;

            write(FilePipes[FilePipeCount][1], &FileCount, sizeof(int));

            read(RSizePipes[RSizePipeCount][0], &RSize, sizeof(int));

            RSize += get_file_size(filepath);

            write(RSizePipes[RSizePipeCount][1], &RSize, sizeof(int));




            if (strcmp(get_filename_ext(hFile->d_name), ".txt") == 0) {

                read(TxtPipes[TxtPipeCount][0], &TxtCount, sizeof(int));

                TxtCount++;

                write(TxtPipes[TxtPipeCount][1], &TxtCount, sizeof(int));
            }
            else if (strcmp(get_filename_ext(hFile->d_name), ".pdf") == 0) {
                read(PdfPipes[PdfPipeCount][0], &PdfCount, sizeof(int));

                PdfCount++;

                write(PdfPipes[PdfPipeCount][1], &PdfCount, sizeof(int));

            }
            else if (strcmp(get_filename_ext(hFile->d_name), ".jpg") == 0) {
                read(JpgPipes[JpgPipeCount][0], &JpgCount, sizeof(int));

                JpgCount++;

                write(JpgPipes[JpgPipeCount][1], &JpgCount, sizeof(int));

            }
            else if (strcmp(get_filename_ext(hFile->d_name), ".png") == 0) {
                read(PngPipes[PngPipeCount][0], &PngCount, sizeof(int));

                PngCount++;

                write(PngPipes[PngPipeCount][1], &PngCount, sizeof(int));

            }


            pthread_mutex_unlock(&mutexCount);



        }
        if (hFile->d_type == DT_DIR) {

            char* temp = (char*)malloc(strlen(filepath) + 1);

            strcpy(temp, filepath);

            printf("folder : %s\n", filepath);

            struct arg* argsPass = (struct arg*)malloc(sizeof(struct arg));

            argsPass->path = temp;

            argsPass->id = id + 1;

            submitThread(argsPass);

        }


    }




    closedir(dirFile);

}



int main()
{

    pthread_mutex_init(&mutexQueue, NULL);

    pthread_mutex_init(&mutexCount, NULL);

    char* p = strdup("");

    int id = 0;

    struct arg* args = (struct arg*)malloc(sizeof(struct arg));

    args->path = p;

    args->id = 0;

    DIR* dirFile;

    if (!(dirFile = opendir(p))) {
        return NULL;
    }

    struct dirent* hFile;

    pid_t pids[10000];

    int procCount = 0;

    pipe(FilePipes[FilePipeCount]);

    write(FilePipes[FilePipeCount][1], &FileCount, sizeof(int));

    pipe(TxtPipes[TxtPipeCount]);

    write(TxtPipes[TxtPipeCount][1], &TxtCount, sizeof(int));

    pipe(PdfPipes[PdfPipeCount]);

    write(PdfPipes[PdfPipeCount][1], &PdfCount, sizeof(int));

    pipe(JpgPipes[JpgPipeCount]);

    write(JpgPipes[JpgPipeCount][1], &JpgCount, sizeof(int));

    pipe(PngPipes[PngPipeCount]);

    write(PngPipes[PngPipeCount][1], &PngCount, sizeof(int));

    char* tempPath = " ";

    int n = strlen(tempPath) + 1;

    pipe(MaxSizePipes[MaxSizePipeCount]);

    write(MaxSizePipes[MaxSizePipeCount][1], &MaxSize, sizeof(long int));

    write(MaxSizePipes[MaxSizePipeCount][1], &n, sizeof(int));

    write(MaxSizePipes[MaxSizePipeCount][1], tempPath, n * sizeof(char));

    char* tempPath_ = " ";

    int n_ = strlen(tempPath) + 1;

    pipe(MinSizePipes[MinSizePipeCount]);

    write(MinSizePipes[MinSizePipeCount][1], &MinSize, sizeof(long int));

    write(MinSizePipes[MinSizePipeCount][1], &n, sizeof(int));

    write(MinSizePipes[MinSizePipeCount][1], tempPath, n * sizeof(char));


    pipe(RSizePipes[RSizePipeCount]);

    write(RSizePipes[RSizePipeCount][1], &RSize, sizeof(int));


    while ((hFile = readdir(dirFile)) != NULL)
    {

        if (strcmp(hFile->d_name, ".") == 0 || strcmp(hFile->d_name, "..") == 0 || hFile->d_name[0] == '.')
            continue;

        char filepath[strlen(p) + 1 + strlen(hFile->d_name) + 1];

        snprintf(filepath, sizeof filepath, "%s/%s", p, hFile->d_name);

        if (hFile->d_type == DT_REG) {

            printf("file : %s\n", hFile->d_name);

            read(FilePipes[0][0], &FileCount, sizeof(int));

            FileCount++;

            write(FilePipes[0][1], &FileCount, sizeof(int));

            read(RSizePipes[0][0], &RSize, sizeof(int));

            RSize += get_file_size(filepath);

            write(RSizePipes[0][1], &RSize, sizeof(int));

            //max

            long int tempMax;

            int tempN;

            read(MaxSizePipes[0][0], &tempMax, sizeof(long int));

            read(MaxSizePipes[0][0], &tempN, sizeof(int));

            char* tempPathh = (char*)malloc(tempN + 1);

            read(MaxSizePipes[0][0], tempPathh, tempN * sizeof(char));

            long int tempM;

            char* temp = (char*)malloc(strlen(filepath) + 1);

            strcpy(temp, filepath);

            tempM = get_file_size(temp);


            if (tempM > tempMax) {

                MaxSize = tempM;

                int tempNN = strlen(temp) + 1;

                write(MaxSizePipes[0][1], &MaxSize, sizeof(long int));

                write(MaxSizePipes[0][1], &tempNN, sizeof(int));

                write(MaxSizePipes[0][1], temp, tempNN * sizeof(char));

            }
            else {

                write(MaxSizePipes[0][1], &tempMax, sizeof(long int));

                write(MaxSizePipes[0][1], &tempN, sizeof(int));

                write(MaxSizePipes[0][1], tempPathh, tempN * sizeof(char));


            }

            //min

            long int tempMin;

            int tempN_;

            read(MinSizePipes[0][0], &tempMin, sizeof(long int));

            read(MinSizePipes[0][0], &tempN_, sizeof(int));

            char* tempPathh_ = (char*)malloc(tempN_ + 1);

            read(MinSizePipes[0][0], tempPathh_, tempN_ * sizeof(char));

            long int tempM_;

            char* temp_ = (char*)malloc(strlen(filepath) + 1);

            strcpy(temp_, filepath);

            tempM_ = get_file_size(temp_);

            if (tempM_ < tempMin) {

                MinSize = tempM_;

                int tempNN_ = strlen(temp_) + 1;

                write(MinSizePipes[0][1], &MinSize, sizeof(long int));

                write(MinSizePipes[0][1], &tempNN_, sizeof(int));

                write(MinSizePipes[0][1], temp_, tempNN_ * sizeof(char));

            }
            else {

                write(MinSizePipes[0][1], &tempMin, sizeof(long int));

                write(MinSizePipes[0][1], &tempN_, sizeof(int));

                write(MinSizePipes[0][1], tempPathh_, tempN_ * sizeof(char));


            }



            if (strcmp(get_filename_ext(hFile->d_name), ".txt") == 0) {

                read(TxtPipes[0][0], &TxtCount, sizeof(int));

                TxtCount++;

                write(TxtPipes[0][1], &TxtCount, sizeof(int));
            }
            else if (strcmp(get_filename_ext(hFile->d_name), ".pdf") == 0) {
                read(PdfPipes[0][0], &PdfCount, sizeof(int));

                PdfCount++;

                write(PdfPipes[0][1], &PdfCount, sizeof(int));

            }
            else if (strcmp(get_filename_ext(hFile->d_name), ".jpg") == 0) {
                read(JpgPipes[0][0], &JpgCount, sizeof(int));

                JpgCount++;

                write(JpgPipes[0][1], &JpgCount, sizeof(int));

            }
            else if (strcmp(get_filename_ext(hFile->d_name), ".png") == 0) {
                read(PngPipes[0][0], &PngCount, sizeof(int));

                PngCount++;

                write(PngPipes[0][1], &PngCount, sizeof(int));

            }




        }
        if (hFile->d_type == DT_DIR) {



            printf("folder : %s\n", filepath);

            if (pids[procCount] == 0) {
                //child

                char* temp = (char*)malloc(strlen(filepath) + 1);

                strcpy(temp, filepath);

                struct arg* argsPass = (struct arg*)malloc(sizeof(struct arg));

                argsPass->path = temp;

                argsPass->id = id + 1;

                submitThread(argsPass);


            }
            else if (pids[procCount] == 0 && getppid() != 0) {
                //parent         

                pid_t temp = fork();

                pids[procCount] = temp;

                FilePipeCount++;

                TxtPipeCount++;

                PdfPipeCount++;

                JpgPipeCount++;

                PngPipeCount++;

                MaxSizePipeCount++;

                RSizePipeCount++;

                pipe(FilePipes[FilePipeCount]);

                write(FilePipes[FilePipeCount][1], &FileCount, sizeof(int));

                pipe(TxtPipes[TxtPipeCount]);

                write(TxtPipes[TxtPipeCount][1], &TxtCount, sizeof(int));

                pipe(PdfPipes[PdfPipeCount]);

                write(PdfPipes[PdfPipeCount][1], &PdfCount, sizeof(int));

                pipe(JpgPipes[JpgPipeCount]);

                write(JpgPipes[JpgPipeCount][1], &JpgCount, sizeof(int));

                pipe(PngPipes[PngPipeCount]);

                write(PngPipes[PngPipeCount][1], &PngCount, sizeof(int));

                pipe(MaxSizePipes[MaxSizePipeCount]);

                char* tempPath = " ";

                int n = strlen(tempPath) + 1;

                write(MaxSizePipes[MaxSizePipeCount][1], &MaxSize, sizeof(long int));

                write(MaxSizePipes[MaxSizePipeCount][1], &n, sizeof(int));

                write(MaxSizePipes[MaxSizePipeCount][1], tempPath, n * sizeof(char));

                char* tempPath_ = " ";

                int n_ = strlen(tempPath) + 1;

                pipe(MinSizePipes[MinSizePipeCount]);

                write(MinSizePipes[MinSizePipeCount][1], &MinSize, sizeof(long int));

                write(MinSizePipes[MinSizePipeCount][1], &n, sizeof(int));

                write(MinSizePipes[MinSizePipeCount][1], tempPath, n * sizeof(char));

                pipe(RSizePipes[RSizePipeCount]);

                write(RSizePipes[RSizePipeCount][1], &RSize, sizeof(int));

                procCount++;




            }


        }

    }



    if (pids[procCount] == 0) {

        for (int j = 0; j < threadCount; j++) {

            if (threads[j] != 0) {

                pthread_join(threads[j], NULL);

            }

        }


    }


    for (int i = 0; i < procCount; i++) {

        waitpid(pids[i], NULL, 0);

    }


    int temp = 0;

    int sum = 0;


    for (int i = 0; i <= FilePipeCount; i++) {

        read(FilePipes[i][0], &temp, sizeof(int));

        sum += temp;



    }


    printf("\n\n\nTotal number of Files : %d\n", sum);


    printf("Number of each file type : \n");

    temp = 0;

    sum = 0;


    for (int i = 0; i <= TxtPipeCount; i++) {

        read(TxtPipes[i][0], &temp, sizeof(int));

        sum += temp;



    }

    printf("- .txt : %d\n", sum);

    temp = 0;

    sum = 0;


    for (int i = 0; i <= PdfPipeCount; i++) {

        read(PdfPipes[i][0], &temp, sizeof(int));

        sum += temp;



    }

    printf("- .pdf : %d\n", sum);

    temp = 0;

    sum = 0;


    for (int i = 0; i <= JpgPipeCount; i++) {

        read(JpgPipes[i][0], &temp, sizeof(int));

        sum += temp;



    }

    printf("- .jpg : %d\n", sum);

    temp = 0;

    sum = 0;


    for (int i = 0; i <= PngPipeCount; i++) {

        read(PngPipes[i][0], &temp, sizeof(int));

        sum += temp;



    }

    printf("- .png : %d\n", sum);

    long int tempMax;

    int tempN;

    char* tempPathh;

    long int MAX = -1;

    char* MAXPATH;

    for (int i = 0; i <= MaxSizePipeCount; i++) {

        read(MaxSizePipes[i][0], &tempMax, sizeof(long int));

        read(MaxSizePipes[i][0], &tempN, sizeof(int));

        tempPathh = (char*)malloc(tempN + 1);

        read(MaxSizePipes[i][0], tempPathh, tempN * sizeof(char));


        if (tempMax > MAX) {
            MAX = tempMax;
            MAXPATH = tempPathh;
        }

    }


    printf("File with the largest size : %s , Size : %ld\n", MAXPATH, MAX);

    long int tempMin;

    int tempN_;

    char* tempPathh_;

    long int MIN = LONG_MAX;

    char* MINPATH;

    for (int i = 0; i <= MinSizePipeCount; i++) {

        read(MinSizePipes[i][0], &tempMin, sizeof(long int));

        read(MinSizePipes[i][0], &tempN_, sizeof(int));

        tempPathh_ = (char*)malloc(tempN_ + 1);

        read(MinSizePipes[i][0], tempPathh_, tempN_ * sizeof(char));


        if (tempMin < MIN) {
            MIN = tempMin;
            MINPATH = tempPathh_;
        }

    }


    printf("File with the smallest size : %s , Size : %ld\n", MINPATH, MIN);



    long int tempSize = 0;

    long long int sumSize = 0;


    for (int i = 0; i <= RSizePipeCount; i++) {

        read(RSizePipes[i][0], &tempSize, sizeof(int));

        sumSize += tempSize;



    }


    printf("Size of the root folder : %d\n", sumSize);



    pthread_mutex_destroy(&mutexQueue);

    pthread_mutex_destroy(&mutexCount);




    return 0;
}