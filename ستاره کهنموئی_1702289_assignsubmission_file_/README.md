### NCDUsimulator
- This C project is a NCDU simulator using a multi-threaded program and it monitores files within a given directory.
- All the threads and proccesses that are made in the program will be shown in the terminal. the files that are monitored will also be shown.
- After the traversal process completes, the main process displays the gathered file information:
  - Total number of files.
  - File types.
  - Addresses of the largest and smallest files with their respective sizes.
  - Final size of the main folder

### Main Function:
- `main()` takes the main directory path as an argument.
- It starts by printing a message indicating that the main process has begun for the main directory.
- It then calls the `traverseDirectoryWithProcesses()` function with the main directory path.
### `traverseDirectoryWithProcesses()`function:
- This function opens the directory specified by the given path and iterates through its contents.
- For each sub-folder found, it creates a child process using `fork()`.
- Each child process then calls `traverseDirectoryWithThreads()` to handle the directory traversal within itself.

### `traverseDirectoryWithThreads()` Function:
- This function operates within each child process, handling the directory traversal with threads.
- It opens the directory specified by the given path and iterates through its contents.
- For each file found (`DT_REG`), it acquires a lock using `pthread_mutex_lock()` to access and modify the shared data.
- Inside the critical section:
  - It updates the file count.
  - Records file types.
  - Calculates file sizes, finding the largest and smallest files, and computes the total size of the main folder.
- After updating the shared data, it releases the lock using `pthread_mutex_unlock()`.

### Shared Memory Usage:
- The `struct FileInfo` structure is used to store file-related information.
- `shmid` represents the shared memory segment, and `sharedData` is a pointer to the shared memory location.
- The shared memory is created using `shmget()` and attached to the process using `shmat()`.

- The code uses shared memory to store file information among different processes.
- `pthread_mutex_t lock` is initialized to manage access to the shared memory data.
- The code detaches and removes the shared memory segment (`shmdt()` and `shmctl()`).
<<<<<<< HEAD
- It also destroys the mutex (`pthread_mutex_destroy()`) after completing the tasks.
=======
- It also destroys the mutex (`pthread_mutex_destroy()`) after completing the tasks.
>>>>>>> 00909507028d417eb9cd5c30c618893bf970c4c5
