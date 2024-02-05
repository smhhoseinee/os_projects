
# OS Midterm Project

OS Course Project for 402-403-1 Semester

Linux NCDU Clone using C Programming Language w/ POSIX Threads and Processes




## How to Use

Run below command in project root

```bash
  main.exe <path>
```
paramter  | required | type | note
------------- | ------------- | ----------- | --------
path  | true | string | path should end with "/" character

## Monitoring

To monitor what threads do, please uncomment line 14 of main.c and recompile the program.

    
## Documentation

1. initDirectoryTask(struct task* task, char input[])
this function iterates over the "input" directory and saves its data on "task"

Data Contains This Information:
paramter  | type | note
------------- | ----------- | --------
filesCount  | int | length of files array
directoryCount | int | length of directory array
extentionsCount | int | length of extensions array
dirSize | ULL | size of current directory
maxFile | struct iFile | file struct of largest file
minFile | struct iFile | file struct of smallest file
directory | array of iDirectory | array of directories
files | array of iFile | array of files
etensions | array of extentionCount | array of extensions

2. *threadFunction(void *arg)
this function recursivly creates threads that we use to analyse subfolders and also calls the initDirectoryTask
after that, it uses the data from initDirectoryTask to update its parents data

arg contains this Information:
paramter  | type | note
------------- | ----------- | --------
task | struct task* | this is the parent's task
threadTask | struct task* | thread task
path | char* | the directory that this thread has to analyse

## Authors

- [Mobina Madanikhah](https://www.github.com/MbnMdn)
- [Erfan Golzari](https://www.github.com/ErfaNEP1)

