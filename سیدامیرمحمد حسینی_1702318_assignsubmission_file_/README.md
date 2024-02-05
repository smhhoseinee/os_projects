
# NCDU simulator for Windows

This project introduces a file monitoring system inspired by Linux NCDU. It carefully examines and reviews files and folders within a given location. Once the analysis is complete, the system presents essential details such as the total folder size, largest and smallest files, overall file count, and types of files in the directory.
# main()
Orchestrates the file monitoring system, setting up shared memory, initializing data structures, and launching processes to traverse directories concurrently. It prints the final results after processing.
# process_directory()
Takes a directory path and a pointer to shared data, then recursively processes its contents using threads for subdirectories. It updates shared information such as file sizes, types, and counts.
# thread_directory()
A thread function executed for each subdirectory, responsible for processing the contents of the directory using the process_directory function.
#add_extension()
Updates the file type count in shared data based on the file extension. It checks if the extension exists and increments its count or adds a new entry if not.