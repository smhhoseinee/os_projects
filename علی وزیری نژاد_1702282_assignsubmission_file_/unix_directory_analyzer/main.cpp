#include "main.h"

int main(int, char**)
{
    string path = "/Users/portg/Desktop/Code/STM32/microprocessor_course/week-8-assignment-duplicate/Core";
    FileInfo info = get_file_info(path);

    // create the shared variable
    string shared_memory_name = "/shared_memory";
    int shared_memory_fd = shm_open(shared_memory_name.c_str(), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(shared_memory_fd, sizeof(DirStat));
    DirStat* shared_stat = (DirStat*) mmap(NULL, sizeof(DirStat), PROT_READ | PROT_WRITE, MAP_SHARED, shared_memory_fd, 0);

    // allocate a semaphore for inter-process synchronization
    string semaphore_name = "/semaphore";
    shared_stat->semaphore_lock = sem_open(semaphore_name.c_str(), O_CREAT, 0644, 1);
    shared_stat->shared_memory_fd = shared_memory_fd;
    shared_stat->shared_memory_name = shared_memory_name;
    shared_stat->semaphore_name = semaphore_name;

    // check for shared memory alocation failure
    if (shared_stat == MAP_FAILED) 
    {
        cerr << "[ERR] Error Mapping Shared Memory";
        return 1;
    }

    // get the root's stats
    *shared_stat = get_dir_stat(info);

    // get level one subdirectories
    vector<FileInfo> sub_dirs; 
    get_sub_dirs(info, OUT sub_dirs);

    // process each subdirectory by a separate process
    process_sub_dirs_by_sub_processes(sub_dirs, *shared_stat);

    // output full statistics
    print_final_result(*shared_stat);

    // perform memory clean ups
    clean_up_process(shared_stat, shared_memory_name, shared_memory_fd, shared_stat->semaphore_lock, 
        shared_stat->semaphore_name);

    // unlink the shared segments
    sem_unlink(shared_stat->semaphore_name.c_str());
    shm_unlink(shared_stat->shared_memory_name.c_str());

    return 0;
}

int clean_up_process(DirStat* shm, string shm_name, int shm_fd, sem_t* sem, string sem_name)
{
    // deallocate the semaphore from the process address space //
    // close
    // if(sem_close(sem) == -1)
    // {
    //     cerr << "[ERR] " << getpid() << ": Error Unmapping the Semaphore on Process Termination" << endl;
    //     return 1;
    // }

    // deallocate shared memory from the process address space //
    // unmap
    if(munmap(shm, sizeof(DirStat)) == -1) {
        cerr << "[ERR] " << getpid() << ": Error Unmapping Shared Memory on Process Termination" << endl;
        return 1;
    }
    // close the file descriptor of the shared memory for this process
    if(close(shm_fd) == -1)
    {
        cerr << "[ERR] " << getpid() << ": Error Closing Shared Memory File Descriptor on Process Termination" << endl;
        return 1;
    }

    cerr << "[INFO] " << getpid() << ": Process Cleaned Up Successully" << endl;

    return 0;
}

int process_sub_dirs_by_sub_processes(const vector<FileInfo>& sub_dirs, DirStat& shared_stat)
{
    // process each subdirectory by a separate process
    vector<pid_t> process_id_list;
    for(auto dir: sub_dirs)
    {
        pid_t pid = fork();

        if(pid == -1) // fail routine
        {
            cerr << "[ERR] Error Forking a New Process" << endl;
            return 1;
        }

        if(pid == 0) // new process routine
        {
            string log = "going in: " + dir.name + "\n";
            cout << log;
            calculate_full_dir_stat(dir, shared_stat);

            // process clean ups
            int exit_status = clean_up_process(&shared_stat, shared_stat.shared_memory_name, shared_stat.shared_memory_fd,
                shared_stat.semaphore_lock, shared_stat.semaphore_name);

            exit(exit_status);
        }

        else // parent process routine
        {
            // store pid to later join all processes
            process_id_list.push_back(pid);
        }
    }

    // wait for all sub-processes to finish
    int status;
    for(auto id: process_id_list)
    {
        waitpid(id, &status, 0);
        cout << "process " << id << " finished: " << status << endl;
    }

    return 0;
}

void print_final_result(const DirStat& stat)
{
    cout << "file count: " << stat.file_count << endl;
    cout << "size: " << stat.size << endl;
    cout << "types:" << endl;
    for(int i = 0; i < stat.file_types_size; ++i)
    {
        cout << stat.file_types[i] << ":" << stat.file_types_count[i] << endl;
    }
    cout << "min file:" << endl;
    cout << stat.min_file_size_path << endl;
    cout << stat.min_file_size << endl;
    cout << "max file:" << endl;
    cout << stat.max_file_size_path << endl;
    cout << stat.max_file_size << endl;
}

int calculate_full_dir_stat(const FileInfo& dir, OUT DirStat& shared_stat)
{
    // cout << "in calculate full dir stat" << endl;
    // calculate level one stat
    DirStat dir_level_one_stat = get_dir_stat(dir);

    // critical section begin //
    sem_wait(shared_stat.semaphore_lock);
    // update the top_level directory stat with new stats
    cout << "lock acquired by: " << getpid() << endl;
    update_dir_stat(dir_level_one_stat, shared_stat);
    cout << "lock released by: " << getpid() << endl;
    sem_post(shared_stat.semaphore_lock);
    // critical section end //

    // get all the sub directories
    vector<FileInfo> sub_dirs;
    get_sub_dirs(dir, OUT sub_dirs);

    // process each subdirectory by a separate thread
    process_sub_dirs_by_threads(sub_dirs, shared_stat);

    return 0;
}

int process_sub_dirs_by_threads(const vector<FileInfo>& sub_dirs, DirStat& shared_stat)
{
    // create a thread for each subdirectory
    vector<pthread_t> threads;
    for(auto sub_dir: sub_dirs)
    {
        // cout << "in thread loop" << endl;
        // create the thread function arguments
        ThreadData thread_data{sub_dir, &shared_stat};

        // create the thread
        pthread_t thread;
        int result  = pthread_create(&thread, NULL, thread_function, &thread_data);

        if (result != 0) 
        {
            std::cerr << "[ERR] Thread Creation Failed: " << result << std::endl;
            return 1;
        }

        else
        {
            threads.push_back(thread);
        }
    }

    // joint threads
    for(auto thread: threads)
    {
        pthread_join(thread, NULL);
    }

    return 0;
}

void* thread_function(void* args)
{
    // unpack thread arguments
    ThreadData& thread_args = *static_cast<ThreadData*>(args);

    // execute thread task
    calculate_full_dir_stat(thread_args.dir, *thread_args.dir_stat);

    // exit thread
    pthread_exit(NULL);
}

void update_dir_stat(const DirStat& from_stat, DirStat& to_stat)
{
    cout << getpid() << ": in update 1" << endl;
    // add new file count
    to_stat.file_count += from_stat.file_count;
    cout << getpid() << ": in update 2" << endl;
    // add new size
    to_stat.size += from_stat.size;
    cout << getpid() << ": in update 3" << endl;
    // add new type records
    for(int i = 0; i < from_stat.file_types_size; ++i)
    {
        cout << getpid() << ": in update 4" << endl;
        // cout << getpid() << ": processing pair: (" << pair.first << "," << pair.second << ")" << endl;
        // check if the record already exists in the shared variable
        auto it = find(to_stat.file_types.begin(), to_stat.file_types.end(), from_stat.file_types[i]);
        if(it != to_stat.file_types.end())
        {
            // cout << getpid() << ": increasing: (" << pair.first << "," << pair.second << ")" << endl;
            // increase the corresponding record
            to_stat.file_count[distance(to_stat.file_types.begin(), it)] += from_stat.file_count[i];
            // (*to_stat.file_type_table_ptr)[pair.first] += pair.second;
        }
        else // insert the new record
        {
            cout << getpid() << ": inserting new one: (" << pair.first << "," << pair.second << ")" << endl;
            // cout << "inserting new one: " << pair.first << endl;
            to_stat.file_type_table_ptr->insert({pair.first, pair.second});
        }
    }
    // update min/max files if necessary
    if(from_stat.max_file_size > to_stat.max_file_size)
    {
        cout << getpid() << ": in update 5" << endl;
        to_stat.max_file_size = from_stat.max_file_size;
        to_stat.max_file_size_path = from_stat.max_file_size_path;
    }
    if(from_stat.min_file_size < to_stat.min_file_size)
    {
        cout << getpid() << ": in update 6" << endl;
        to_stat.min_file_size = from_stat.min_file_size;
        to_stat.min_file_size_path = from_stat.min_file_size_path;
    }
    cout << getpid() << ": in update 7" << endl;
}

DirStat get_dir_stat(const FileInfo& dir_info)
{
    DIR* dir = opendir(dir_info.path.c_str());
    dirent* entry;
    struct stat file_stat;

    // stats to be collected
    uint64_t file_count = 0;
    uint64_t size = 0;
    vector<string> file_types;
    vector<int> file_types_count;
    string min_file_size_path = "empty";
    string max_file_size_path = "empty";
    uint64_t min_file_size = numeric_limits<uint64_t>::max();
    uint64_t max_file_size = 0;

    while((entry = readdir(dir)) != nullptr)
    {
        // skip the . and .. entries
        if((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
        {
            continue;
        }        

        string sub_path = dir_info.path + "/" + entry->d_name;        
        // get the file info
        FileInfo info = get_file_info(sub_path);

        // increment file count
        ++file_count;

        // increase dir size
        size += info.size;

        // check for max/min size
        if(info.size > max_file_size)
        {
            // update max file size
            max_file_size = info.size;
            max_file_size_path = info.path;
        }
        if(info.size < min_file_size)
        {
            // update min file size
            min_file_size = info.size;
            min_file_size_path = info.path;
        }

        // add the file extension to the file type table
        auto it = find(file_types.begin(), file_types.end(), info.type);
        if(it != file_types.end())
        {
            ++file_types_count[distance(file_types.begin(), it)];
        }
        else
        {
            file_types.push_back(info.type);
            file_types_count.push_back(1);
        }
    }


    closedir(dir);

    DirStat dir_stat
    {
        .file_count = file_count,
        .size = size,
        .file_types{},
        .file_types_count{},
        .file_types_size = 0,
        .max_file_size = max_file_size,
        .max_file_size_path = max_file_size_path,
        .min_file_size = min_file_size,
        .min_file_size_path = min_file_size_path
    };

    for(int i = 0; i < file_types.size(); ++i)
    {
        dir_stat.file_types[dir_stat.file_types_size] = file_types[i];
        dir_stat.file_types_count[dir_stat.file_types_size] = file_types_count[i];
        ++dir_stat.file_types_size;
    }

    return dir_stat;
}

void get_sub_dirs(const FileInfo& dir_info, OUT vector<FileInfo>& sub_dirs)
{
    string path = dir_info.path;

    // remove the trailing slash (will be appended later)
    if(path[path.size() - 1] == '/')
    {
        path = path.substr(0, path.size() - 1);
    }
    
    DIR* dir = opendir(path.c_str());
    dirent* entry;
    struct stat file_stat;

    // loop through all the files whithin the directory
    while((entry = readdir(dir)) != nullptr)
    {
        // skip the . and .. entries
        if((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
        {
            continue;
        }

        string subdir_path = path + "/" + entry->d_name;
        // take the stats of the read file
        if(stat(subdir_path.c_str(), &file_stat) == 0)
        {
            // check if the file is a directory
            if(S_ISDIR(file_stat.st_mode))
            {
                sub_dirs.push_back(get_file_info(subdir_path));
            }
        }
    }

    closedir(dir);
}

FileInfo get_file_info(string path)
{
    // extract the name //

    string name = "";
    bool append_slash = false;

    // remove the trailing slash (will be appended later)
    if(path[path.size() - 1] == '/')
    {
        path = path.substr(0, path.size() - 1);
        append_slash = true;
    }

    // find the last slash character where the name begins
    int last_slash_index = path.find_last_of("/");
    // check if there is no slash in the path
    if(last_slash_index == string::npos)
    {
        name = path;
    }

    // extract the name
    else
    {
        int name_first_index = last_slash_index + 1;
        name = path.substr(last_slash_index + 1);
    }

    // append the trailing slash
    if(append_slash)
    {
        name += "/";
    }

    // extract the extension //

    string extension = "no extension";
    struct stat file_stat;
    stat(path.c_str(), &file_stat);

    // check whether it is a directory
    if(S_ISDIR(file_stat.st_mode))
    {
        extension = "dir";
    }

    else // it is a file
    {
        int last_period_index = name.find_last_of(".");

        // check if there is an extension
        if((last_period_index != string::npos) && (last_period_index != name.size() - 1))
        {
            string text_after_period = name.substr(last_period_index);

            // validate that the extension doesn't contain whitespace
            if(text_after_period.find_first_of(" ") == string::npos)
            {
                extension = text_after_period;
            }
        }
    }

    // get the size //

    uint64_t size = file_stat.st_size;
    FileInfo info{name, path, size, extension};

    return info;
}