#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <vector>
#include <map>
using namespace std;

const int MAX_LEVELS = 10;
const size_t SHARED_MEMORY_SIZE = 4096;  // Adjusted size for larger paths

struct MapItem {
    string key;
    int value;
};

struct SharedData {
    int counter;
    char biggestItemPath[SHARED_MEMORY_SIZE];
    char smallestItemPath[SHARED_MEMORY_SIZE];
    off_t biggestItemSize;
    off_t smallestItemSize;
    pthread_mutex_t mutex;
    int totalItems;
    off_t totalSize;
    int mapCounter;
    MapItem fileTypeCounts[SHARED_MEMORY_SIZE];
};

// Struct to pass data to the thread
struct ThreadData {
    string directoryPath;
    SharedData* sharedData;
};

int initSharedMemory(SharedData*& sharedData, int& shm_fd) {
    shm_fd = shm_open("/my_shared_memory", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        cerr << "Error creating shared memory" << endl;
        return EXIT_FAILURE;
    }

    ftruncate(shm_fd, sizeof(SharedData));
    sharedData = static_cast<SharedData*>(mmap(nullptr, sizeof(SharedData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0));

    if (sharedData == MAP_FAILED) {
        cerr << "Error mapping shared memory in main process" << endl;
        return EXIT_FAILURE;
    }

    sharedData->counter = 0;
    strncpy(sharedData->biggestItemPath, "", SHARED_MEMORY_SIZE);
    strncpy(sharedData->smallestItemPath, "", SHARED_MEMORY_SIZE);
    sharedData->biggestItemSize = 0;
    sharedData->smallestItemSize = numeric_limits<off_t>::max();
    sharedData->totalSize = 0;
    sharedData->totalItems = 0;
    sharedData->mapCounter = 0;
    
    return EXIT_SUCCESS;
}


int initMutex(pthread_mutex_t& mutex) {
    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_init(&mutexAttr);
    pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&mutex, &mutexAttr);
    pthread_mutexattr_destroy(&mutexAttr);

    return EXIT_SUCCESS;
}

void cleanup(SharedData* sharedData, int shm_fd) {
    pthread_mutex_destroy(&sharedData->mutex);
    munmap(sharedData, sizeof(SharedData));
    shm_unlink("/my_shared_memory");
}

// Function to update item statistics
void updateItemStats(const char* itemPath, off_t itemSize, SharedData* sharedData) {
     
    while (pthread_mutex_trylock(&sharedData->mutex) != 0) {
        // Lock not acquired, wait for a short duration before trying again
        usleep(10000);  // Sleep for 10 milliseconds (adjust as needed)
    }

    if (itemSize > sharedData->biggestItemSize) {
        sharedData->biggestItemSize = itemSize;
        strncpy(sharedData->biggestItemPath, itemPath, SHARED_MEMORY_SIZE);
    }
    

    if (itemSize < sharedData->smallestItemSize) {
        sharedData->smallestItemSize = itemSize;
        strncpy(sharedData->smallestItemPath, itemPath, SHARED_MEMORY_SIZE);
    }

    sharedData->totalSize += itemSize;
    sharedData->totalItems++;

    // Update file type counts
    size_t dotPos = std::string(itemPath).find_last_of(".");
    if (dotPos != std::string::npos) {
        std::string fileType = std::string(itemPath).substr(dotPos + 1);
        bool check=false;
        for (int i=0;i<sharedData->mapCounter;i++ ) 
            if(sharedData->fileTypeCounts[i].key==fileType)
                sharedData->fileTypeCounts[i].value++,check=true;
        if(!check){
            sharedData->fileTypeCounts[sharedData->mapCounter].key=fileType;
            sharedData->fileTypeCounts[sharedData->mapCounter].value=1;
            sharedData->mapCounter++;
        }
    }



    pthread_mutex_unlock(&sharedData->mutex);
}

// Function to process non-folder files in a directory using a thread
void* processDirectoryThread(void* data) {
    pthread_t thread_id = pthread_self();
    ThreadData* threadData = static_cast<ThreadData*>(data);
    string directoryPath = threadData->directoryPath;
    SharedData* sharedData = threadData->sharedData;

    DIR* dir = opendir(directoryPath.c_str());
    if (dir == nullptr) {
        cerr << "Error opening directory: " << directoryPath << endl;
        delete threadData;  // Clean up thread data
        return nullptr;
    }

    struct dirent* entry;
    string filenames[10010];
    int i =0;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG) {
            filenames[i++]=entry->d_name;
            // Regular file, process it
            string itemPath = directoryPath + "/" + entry->d_name;
            struct stat itemStat;

            if (stat(itemPath.c_str(), &itemStat) == 0) {
                updateItemStats(itemPath.c_str(), itemStat.st_size, sharedData);
            }
        }
    }
    cout << "    Thread ID: " << thread_id
         << "\n    searched for files in "<<directoryPath <<endl;
    for(int j =0;j<i;j++){
        cout << "      - "<<filenames[j]<<endl ;
    }
    cout << endl;
    closedir(dir);
    delete threadData;  // Clean up thread data
    return nullptr;
}


void processDirectory(const string& path, int level, int maxLevels, SharedData* sharedData, int shm_fd) {
    cout << "Process ID: " << getpid() 
         << "\n  Level " << level 
         << "\n  Directory: " << path 
         << endl<< endl;


    if (level < maxLevels) {
        DIR* dir = opendir(path.c_str());
        if (dir == nullptr) {
            cerr << "Error opening directory: " << path << endl;
            return;
        }

        //creating thread to do calculations in this dir
        ThreadData* threadData = new ThreadData{path, sharedData};
        pthread_t thread;
        if (pthread_create(&thread, nullptr, processDirectoryThread, static_cast<void*>(threadData)) != 0) {
            cerr << "Error creating thread for directory: " << path << endl;
            delete threadData;  // Clean up thread data
        } 


        vector <pid_t> children;
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                // this is a sub dir
                string subdirectoryPath = path + "/" + entry->d_name;
                pid_t childPid = fork();
                 if (childPid < 0) {
                    cerr << "Error forking process for subdirectory: " << subdirectoryPath << endl;
                    closedir(dir);
                    return;
                } else if (childPid == 0) {
                    // This is the child process
                    processDirectory(subdirectoryPath, level + 1, maxLevels, sharedData, shm_fd);
                    closedir(dir);
                    exit(EXIT_SUCCESS);
                } else {
                    children.push_back(childPid);
                }
            }
        }
        // Wait for thread to finish
        pthread_join(thread, nullptr);


        for( int i=0; i < children.size();i++){
            int status;
            waitpid(children[i], &status, 0);
        }
        closedir(dir);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <directory_path>" << endl;
        return EXIT_FAILURE;
    }

    string initialDirectory = argv[1];

    SharedData* sharedData = nullptr;
    int shm_fd;

    // Initialize shared memory
    if (initSharedMemory(sharedData, shm_fd) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    // Initialize the mutex
    if (initMutex(sharedData->mutex) != EXIT_SUCCESS) {
        cleanup(sharedData, shm_fd);
        return EXIT_FAILURE;
    }

    // Process the initial directory recursively
    processDirectory(initialDirectory, 0, MAX_LEVELS, sharedData, shm_fd);

    // Display the final statistics
    cout << "\nFinal Statistics:\n"
         << "Total Items: " << sharedData->totalItems <<" (Size: " << sharedData->totalSize << " bytes)\n"
         << "Biggest Item: " << sharedData->biggestItemPath << " (Size: " << sharedData->biggestItemSize << " bytes)\n"
         << "Smallest Item: " << sharedData->smallestItemPath << " (Size: " << sharedData->smallestItemSize << " bytes)\n"
         << "File Type Counts:\n";

    for (int i=0;i<sharedData->mapCounter;i++ ) {
        cout << "  - ."<<sharedData->fileTypeCounts[i].key << " : " << sharedData->fileTypeCounts[i].value << " \n";
    }

    // Clean up shared memory and mutex
    cleanup(sharedData, shm_fd);

    return 0;
}
