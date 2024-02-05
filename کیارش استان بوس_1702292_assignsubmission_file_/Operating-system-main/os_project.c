#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <inttypes.h>
#include <gtk/gtk.h>

typedef struct {
    int value1;
    int value2;
    char *message;
} UserData;
// this arguments is information about file directory.
struct arg_function
{
    char dir[1024];
    int depth;
    long int root;
    long int maximum;
    long int minimum;
    char min_directory[1024];
    char max_directory[1024];
    int number_of_files;
    int text_filse;
    int png_files;
    int jpg_filse;
    int zip_filse;
    int mp4_filse;
    int pptx_filse;
    int pdf_files;
    int c_filse;
    int uknown_filse;
};

pthread_mutex_t lock1; // lock varible

// this function convert byte -> kilo byte -> mega byte -> giga byte.
void calculateSize(size_t size)
{   
  static const char *SIZES[] = { "B", "kB", "MB", "GB" };
    size_t div = 0;
    size_t rem = 0;

    while (size >= 1024 && div < (sizeof SIZES / sizeof *SIZES)) {
        rem = (size % 1024);
        div++;   
        size /= 1024;
    }

    printf("%.2f %s\n", (float)size + (float)rem / 1024.0, SIZES[div]);
}
float calculateSize_noPrint(size_t size)
{   
  static const char *SIZES[] = { "B", "kB", "MB", "GB" };
    size_t div = 0;
    size_t rem = 0;

    while (size >= 1024 && div < (sizeof SIZES / sizeof *SIZES)) {
        rem = (size % 1024);
        div++;   
        size /= 1024;
    }

    return((float)size + (float)rem / 1024.0);
}

// this function return size of file.
long int findSize(char file_name[]) 
{ 
    // opening the file in read mode 
    FILE* fp = fopen(file_name, "r"); 
  
    // checking if the file exist or not 
    if (fp == NULL) { 
        printf("File Not Found!\n"); 
        return -1; 
    } 
  
    fseek(fp, 0L, SEEK_END); 
  
    // calculating the size of the file 
    long int res = ftell(fp); 
  
    // closing the file 
    fclose(fp); 
  
    return res; 
} 


void recurive_find_files(char *dir, struct arg_function *arguments)
{
    char s[1024]; //->for live direction
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    if((dp = opendir(dir)) == NULL) {   // -> sure about write directory
        fprintf(stderr,"cannot open directory: %s\n", dir);
        return;
    }

    chdir(dir);

    while((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name,&statbuf);
        if(S_ISDIR(statbuf.st_mode)) {
            /* Found a directory, but ignore . and .. */
            if(strcmp(".",entry->d_name) == 0 || strcmp("..",entry->d_name) == 0)
                continue;

            /* Recurse at a new indent level */

            recurive_find_files(entry->d_name, arguments);

        }
        else{ //we found file not folder

            // Update statistics using a lock to ensure synchronization
            pthread_mutex_lock(&lock1); // if we dont have somthing like this then the programm giving somthingg incorrect...
            // start critical section

            arguments->number_of_files++;   // total number of files ++ we file


            //update root size
            arguments->root += findSize(entry->d_name);

            //update maximum
            if(findSize(entry->d_name) > arguments->maximum)
            {
                arguments->maximum = findSize(entry->d_name);
                memcpy(arguments->max_directory, getcwd(s, 1024), strlen(getcwd(s, 1024)));
                strcat(arguments->max_directory, "/");
                strcat(arguments->max_directory, entry->d_name);
            }

            //update minimum
            if(findSize(entry->d_name) < arguments->minimum)
            {
                arguments->minimum = findSize(entry->d_name);
                memcpy(arguments->min_directory, getcwd(s, 1024), strlen(getcwd(s, 1024)));
                strcat(arguments->min_directory, "/");
                strcat(arguments->min_directory, entry->d_name);
            }

            //check what type of file we have.
            if(strstr(entry->d_name, ".txt")!=NULL)
                arguments->text_filse++;
            else if(strstr(entry->d_name, ".png")!=NULL)
                arguments->png_files++;
            else if(strstr(entry->d_name, ".jpg")!=NULL)
                arguments->jpg_filse++;
            else if(strstr(entry->d_name, ".pdf")!=NULL)
                arguments->pdf_files++;
            else if(strstr(entry->d_name, ".mp4")!=NULL)
                arguments->mp4_filse++;
            else if(strstr(entry->d_name, ".zip")!=NULL)
                arguments->zip_filse++;
            else if(strstr(entry->d_name, ".pptx")!=NULL)
                arguments->pptx_filse++;
            else if(strstr(entry->d_name, ".c")!=NULL)
                arguments->c_filse++;
            else
                arguments->uknown_filse++;
            
            pthread_mutex_unlock(&lock1); // exit critical section

        }
    }
    chdir("..");
    closedir(dp);
}

void *thread_func(void *arg) {

    char *path = (char *)arg;
    struct arg_function *arguments = malloc(sizeof(struct arg_function));
    memset(arguments, 0, sizeof(struct arg_function));
    arguments->maximum = 0;
    arguments->minimum = 429496729;
    arguments->root = 0;
    recurive_find_files(path, arguments);
    pthread_exit(arguments);
    
}

int main(int argc, char **argv)
{
    //// graphical choose
     char command[100] = "zenity --file-selection --directory";
    char check_directory[1024]; 
    FILE* file = popen(command, "r");
    if (file == NULL) {
        printf("Failed to open file explorer.\n");
        return 1;
    }

    char selected_directory[1024];
    fgets(selected_directory, sizeof(selected_directory), file);

    pclose(file);

    printf("Selected directory: %s\n", selected_directory);
    //////

    char s[1024]; 
    
    // start trying get input -> go back to home
    while(s[1] == 'h')
    {
        chdir(".."); // -> back to home
        getcwd(s, 1024); // ->  new s
    }
    // get input

    strtok(selected_directory, "\n");
    strcpy(check_directory , selected_directory);
    pthread_mutex_init(&lock1, NULL);

    pthread_t thread;

    if (pthread_create(&thread, NULL, thread_func, check_directory) != 0) { //create
        perror("Error creating thread");
        exit(EXIT_FAILURE);
    }
    struct arg_function *arguments;
    if (pthread_join(thread, (void **)&arguments) != 0) { //join
        perror("Error joining thread");
        exit(EXIT_FAILURE);
    }

    //show result program
    printf("root size is : \n");
    calculateSize(arguments->root);

    printf("\n");

    printf("file with the smallest size :\n");
    puts(arguments->min_directory);
    calculateSize(arguments->minimum);
    
    printf("\n");

    printf("file with the largest size :\n");
    puts(arguments->max_directory);
    calculateSize(arguments->maximum);

    printf("\n");

    printf("the total number of files:  %d\n", arguments->number_of_files);
    printf("text_files :  %d\n", arguments->text_filse);
    printf("png_files :  %d\n", arguments->png_files);
    printf("jpg_files :  %d\n", arguments->jpg_filse);
    printf("pdf_files :  %d\n", arguments->pdf_files);
    printf("mp4_files :  %d\n", arguments->mp4_filse);
    printf("zip_files :  %d\n", arguments->zip_filse);
    printf("pptx_files :  %d\n", arguments->pptx_filse);
    printf("c_files :  %d\n", arguments->c_filse);
    printf("uknown_ffiles :  %d\n", arguments->uknown_filse);
    int cc_files = arguments->c_filse;
    free(arguments);

    pthread_mutex_destroy(&lock1);

//      /home/kiarash/Desktop/Main
// show result graphically

  GtkWidget *window;
    GtkWidget *label;
    int root=calculateSize_noPrint(arguments->root);

    char* min=arguments->min_directory;
    int minn=calculateSize_noPrint(arguments->minimum);
    
    char* max=arguments->max_directory;
    int maxx=calculateSize_noPrint(arguments->maximum);

    int total=arguments->number_of_files;
     int text_filse = arguments->text_filse;
     int png_files = arguments->png_files;
     int jpg_filse = arguments->jpg_filse;
     int pdf_files = arguments->pdf_files;
     int mp4_filse = arguments->mp4_filse;
     int zip_filse = arguments->zip_filse;
     int pptx_filse = arguments->pptx_filse;
     //int cc_files = arguments->c_filse;
     int uknown_filse = arguments->uknown_filse;

    // "<b>output:</b>\n\n root Size: %d\n minimum directory: %s\n minimum size:
    //  %d\n maximum directory: %s\n maximum size: %d\n total files: %d\n text files: %d
    //  \n png files: %d\n jpg files: %d\n pdf files: %d\n mp4 files: %d\n zip files: %d
    //  \n pptx files: %d\n c_files files: %d\n unknown files: %d", root, min, minn,max,maxx,total,text_filse,png_files,jpg_filse,pdf_files,
    //  mp4_filse,zip_filse,pptx_filse,c_filse,uknown_filse

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "OS project");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), g_strdup_printf( "<b>Result:</b>\n\n root Size: %d MB\n minimum directory: %s\n minimum size:%d MB \n maximum directory: %s\n maximum size: %d MB\n total files: %d\n text files: %d\n png files: %d\n jpg files: %d\n pdf files: %d\n mp4 files: %d\n zip files: %d\n pptx files: %d\n c_files : %d\n unknown files: %d", root, min, minn,max,maxx,total,text_filse,png_files,jpg_filse,pdf_files,mp4_filse,zip_filse,pptx_filse,cc_files,uknown_filse));
    gtk_container_add(GTK_CONTAINER(window), label);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);

    gtk_main();
/// end of graphical output
    return 0;
}