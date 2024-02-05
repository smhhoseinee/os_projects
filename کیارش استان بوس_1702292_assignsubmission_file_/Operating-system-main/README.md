# Operating-system
implanted for OS Project

just run the code and you can choose the directory.

The program will return the total number of each type of file,max size,min size of the chosen directory and it's subdirectories.

the calculateSize function -> takes a size value, divides it by 1024 until it reaches a size less than 1024, and then prints the converted size along with the appropriate unit label (e.g., 1.23 MB).

the find_size function -> opens a file, checks if it exists, determines its size, and returns the size in bytes. If the file doesn't exist or there is an error opening it, the function returns -1.

the recursive_find_files function -> recursively traverses a directory and its subdirectories, analyzing files and updating various statistics such as the total number of files, root size, maximum and minimum file sizes, and file type counters. It uses file system functions and structures to retrieve file information and performs thread-safe updates using a mutex to ensure synchronization.

the thread_func function -> is the entry point for a thread. It receives a directory path as an argument, initializes a structure for holding file statistics, calls the recurive_find_files function to analyze files and update the statistics, and then exits the thread, returning the arguments structure.

Graphical output implanted with GTK
