

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <wait.h>
#include <gtk/gtk.h>

#define MAX_FILE_TYPES 30

void on_button_clicked(GtkWidget *widget, gpointer data);
void* count_file(void *arguments);
void child_processes(char* path);

typedef struct {
    char path[512];  // Increase the buffer size
    char type[10];   // Increase the buffer size for file types
    off_t size;
} FileInfo;

typedef struct
{
  int tab;
  char* path;
}arguments;

typedef struct{
  FileInfo max_size;
  FileInfo min_size;
  int fileCountByType[MAX_FILE_TYPES];
  char fileTypes[MAX_FILE_TYPES][10];
  int total_file;
  off_t root_folder_size;
} result;

result *rslt;

pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]) {
  
  //////////////////////////////////////////////////
  GtkWidget *window;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *vbox;

  gtk_init(&argc, &argv);
  

  // Create the main window
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Directory Selector");
  gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);

  // Create a vertical box layout to hold the label and button
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

  // Create the label
  label = gtk_label_new("Selected Directory: ");
  gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

  // Create a button to open the file chooser dialog
  button = gtk_button_new_with_label("Open Directory");
  g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), label);
  gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, TRUE, 0);

  // Add the layout to the main window
  gtk_container_add(GTK_CONTAINER(window), vbox);

  // Connect the "destroy" signal of the window to terminate the GTK main loop
  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  // Show all widgets
  gtk_widget_show_all(window);

  // Start the GTK main loop
  gtk_main();

  return 0;
}

void on_button_clicked(GtkWidget *widget, gpointer data) {
  rslt = mmap (NULL, _SC_PAGE_SIZE, PROT_READ| PROT_WRITE
                        , MAP_SHARED | MAP_ANONYMOUS, -1, 0 );
  rslt->total_file =0;

  rslt->max_size.size=0;
  strcpy(rslt->max_size.path,"");
  strcpy(rslt->max_size.type,"");
  rslt->min_size.size=LLONG_MAX;
  strcpy(rslt->min_size.path,"");
  strcpy(rslt->min_size.type,"");

  rslt->root_folder_size=0;

  int i;
  for (i=0 ; i<MAX_FILE_TYPES ; i++) {
    rslt->fileCountByType[i]=0;
    strcpy(rslt->fileTypes[i],"");
  }

  /////////////////////////////////////////////////////////////

    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
    gint res;

    // Create a new file chooser dialog
    dialog = gtk_file_chooser_dialog_new("Select a Directory",
                                         NULL,
                                         action,
                                         "_Cancel",
                                         GTK_RESPONSE_CANCEL,
                                         "_Open",
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

    // Run the dialog and get the response
    res = gtk_dialog_run(GTK_DIALOG(dialog));

    if (res == GTK_RESPONSE_ACCEPT) {
        char *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        filename = gtk_file_chooser_get_filename(chooser);
        
        child_processes(filename);

        // Get the label widget
        GtkWidget *label = GTK_WIDGET(data);

        char labelText[2048];
        snprintf(labelText, sizeof(labelText),
                 "Total number of files: %d\n"
                 "Number of each file type:\n", rslt->total_file);
        
        for (i = 0; i < MAX_FILE_TYPES; i++) {
          if(rslt->fileCountByType[i] > 0){
            snprintf(labelText + strlen(labelText), sizeof(labelText) - strlen(labelText),
                     "- %s: %d\n", rslt->fileTypes[i], rslt->fileCountByType[i]);
          }else{
            break;
          }
        }


        snprintf(labelText + strlen(labelText), sizeof(labelText) - strlen(labelText),
                 "File with the largest size: %s, Size: %lld bytes\n"
                 "File with the smallest size: %s, Size: %lld bytes\n"
                 "Size of the root folder: %.2f KB",
                 rslt->max_size.path, (long long)rslt->max_size.size,
                 rslt->min_size.path, (long long)rslt->min_size.size,
                 (double)rslt->root_folder_size/(1024));





        // Update the label with the selected directory path
        gtk_label_set_text(GTK_LABEL(label), labelText);

    }

  /////////////////////////////////////////////////////////////
  

  printf("Total number of files: %d\n", rslt->total_file);
  for (i = 0; i < MAX_FILE_TYPES; i++) {
      if (rslt->fileCountByType[i] > 0) {
          printf("- .%s: %d\n", rslt->fileTypes[i], rslt->fileCountByType[i]);
      } else {
        break;
      }
  }
  printf("File with the largest size: %s, Size: %lld B\n", rslt->max_size.path, (long long)rslt->max_size.size );
  printf("File with the smallest size: %s, Size: %lld B\n", rslt->min_size.path, (long long)rslt->min_size.size );
  printf("Size of the root folder: %.2f KB\n", (double)rslt->root_folder_size/(1024));

  ///////////////////////////////////////////////////////////////
    
    // Destroy the dialog
    gtk_widget_destroy(dialog);
}


void child_processes(char* folderPath){

  DIR *directory;

  struct dirent *entry;
  
  directory = opendir(folderPath);

  struct stat file_stat;

  int i, j=0;
  int first=0;  
  pid_t parentPid;

  char p[256] ;
  strcat(p, " "); strcpy(p, folderPath);
  

  char* token = strtok(p, "/");
  char* root = "";
  while (token != NULL) {
    if(first) {
      root = token;
    }
    token = strtok(NULL, "/");  
  }

  printf("\n%s -> Parent Process\n", root);
  
  while ((entry = readdir(directory)) != NULL)
  {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    char path_goal[512];
    strcpy(path_goal, folderPath);
    strcat(path_goal, "/");
    strcat(path_goal, entry->d_name);
    
    if (entry->d_type == DT_REG)
    {
      pthread_mutex_lock(&lock1);

      (rslt->total_file)++;

      printf("\tFile: %s\n", entry->d_name);

      char* token = strtok(entry->d_name, ".");
      char* format = "";
      first =0;

      while (token != NULL) {
        if (first)
          format=token;
        token = strtok(NULL, ".");  
        first=1;
      }

      stat(path_goal,&file_stat);
      (rslt->root_folder_size) += file_stat.st_size;

      if (file_stat.st_size > (rslt->max_size.size)) {
        strcpy((rslt->max_size.path),path_goal);
        if (format!=NULL) {
          strcpy((rslt->max_size.type),format);
        }
        (rslt->max_size.size) = file_stat.st_size;
      }

      if (file_stat.st_size < (rslt->min_size.size)) {
        strcpy((rslt->min_size.path),path_goal);
        if (format!=NULL) {
          strcpy((rslt->min_size.type),format);
        }

        (rslt->min_size.size) = file_stat.st_size;
      }


      for (i = 0; i < MAX_FILE_TYPES; i++) {
          if (format == NULL) {
              // File type is empty, consider it as an unknown type
              (rslt->fileCountByType[i])++;
              break;
          } else if (strcmp(format, rslt->fileTypes[i]) == 0) {
              (rslt->fileCountByType[i])++;
              //new_format = 0;
              break;
          } else if ((rslt->fileCountByType[i]) == 0) {
              // Encountered a new file type, update the array
              strncpy(rslt->fileTypes[i], format, sizeof(rslt->fileTypes[i]));
              (rslt->fileCountByType[i])++;
              break;
          }
      }
      pthread_mutex_unlock(&lock1);
    }
    
    else if (entry->d_type == DT_DIR)
    {
      
      printf("\tDir: %s-> child process %d\n", entry->d_name, j+1); j++;
      pid_t childPid = fork();
      
      
      if(childPid == 0) {
        arguments arg;
        arg.path = path_goal;
        arg.tab =2;
        count_file((void*)&arg);
        exit(EXIT_FAILURE);
      }
    }
  }
  sleep(1);
}

void* count_file(void *argu)
{
  arguments *arg = (arguments *)argu;

  char* folderPath = arg->path;
  int tab = arg->tab;

  DIR *directory;

  struct dirent *entry;
  
  directory = opendir(folderPath);

  struct stat file_stat;

  int i, j=0;
  int first;

  char pathes[20][512]={""};
  pthread_t array[100];
  
  
  
  while ((entry = readdir(directory)) != NULL)
  {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    char path_goal[512];
    strcpy(path_goal, folderPath);
    strcat(path_goal, "/");
    strcat(path_goal, entry->d_name);
    
    if (entry->d_type == DT_REG)
    {
      pthread_mutex_lock(&lock2);

      (rslt->total_file)++;
      for (i=0 ; i<tab; i++) {
        printf("\t");
      }
      printf("File: %s -> thread:%d\n", entry->d_name, getpid());

      char* token = strtok(entry->d_name, ".");
      char* format = "";
      first =0;

      while (token != NULL) {
        if (first)
          format=token;
        token = strtok(NULL, ".");  
        first=1;
      }

      stat(path_goal,&file_stat);
      (rslt->root_folder_size) += file_stat.st_size;

      if (file_stat.st_size > (rslt->max_size.size)) {
        strcpy(rslt->max_size.path,path_goal);
        if (format!=NULL) {
          strcpy(rslt->max_size.type,format);
        }
        rslt->max_size.size = file_stat.st_size;
      }

      if (file_stat.st_size < (rslt->min_size.size)) {
        strcpy(rslt->min_size.path,path_goal);
        if (format!=NULL) {
          strcpy(rslt->min_size.type,format);
        }

        rslt->min_size.size = file_stat.st_size;
      }


      for (i = 0; i < MAX_FILE_TYPES; i++) {
          if (format == NULL) {
              // File type is empty, consider it as an unknown type
              (rslt->fileCountByType[i])++;
              break;
          } else if (strcmp(format, rslt->fileTypes[i]) == 0) {
              (rslt->fileCountByType[i])++;
              //new_format = 0;
              break;
          } else if (rslt->fileCountByType[i] == 0) {
              // Encountered a new file type, update the array
              strncpy(rslt->fileTypes[i], format, sizeof(rslt->fileTypes[i]));
              (rslt->fileCountByType[i])++;
              break;
          }
      }

      pthread_mutex_unlock(&lock2);
    }
    
    else if (entry->d_type == DT_DIR)
    {
      for (i=0 ; i<tab; i++) {
        printf("\t");
      }
      
      arguments ar;
      ar.path = path_goal; ar.tab= tab+1;
      pthread_t thread;

      printf("Dir: %s\n", entry->d_name);
      
      pthread_create(&thread, NULL, count_file, (void *) &ar);
      pthread_join(thread, NULL);
      
    }
  }
}

/* 
cd project
cd ReyhanehYeganeh_4001262240_midtermProjectOS
cd GUI
unset GTK_PATH
gcc midterm_project_GUI.c `pkg-config --cflags gtk+-3.0 --libs gtk+-3.0`
./a.out
*/