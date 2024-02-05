# **File alanyser**

The aim of this project is to simply analyse folders and files of a directory using child process and threads.

## Run the code

To run our code we should open a terminal in the .exe file of the project and enter this command:

```bash
  main.exe<directory>
```

First, in the main function, we have called the first_task function to access the items in the address with the address received from the user, and in this function we assign a process to each existing folder for each of these processes.  
We call the directory_task function, which assigns a thread to each subfolder in the process, and directory_task is called in the function related to threads (threadFun) so that this action continues to the depth and until the subfolder is no longer If there is no folder, it assigns threads to the folders.  
For the files that are seen in the first_task and directory_task function at the corresponding address, in these calculation functions it is necessary to be able to obtain the following:  
1 Total number of files  
2. The number of types of files  
3. The address of the file with the largest size + its size value  
4. The address of the file with the smallest size + its size value  
5. The final size of the root folder  
For these actions, we used the result structure to store these values in it, and we used shared memory to update and access processes and threads to these values, and we used mutex to synchronize them.  
The isDir function determines whether the address is a file or a directory by taking the address.  
The make_path function creates the parent address + / + the filename or directory with strcat to use it in, for example, the isDir function.  
To get the number of file types, we have two functions, firstly, the file type is obtained with the get_type function, and then with the addType function, according to the type of data returned by get_type, we check that if in the array of file types (fileTypes) If this type is available, increase its number component by one unit, and if it is not available, add this type and set its value to one.
