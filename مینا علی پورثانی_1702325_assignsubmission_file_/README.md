# OS_midterm_project
# File and Folder Information Utility

This C program is designed to process a folder and gather various information about its contents, such as file and folder counts, maximum and minimum file sizes, total folder size, and counts of different file types.

## How it Works

The program uses multithreading to scan the folder and its subdirectories concurrently.
 The main features include:
- Creating a thread to process each subdirectory of the provided folder.
- Gathering file-specific information such as count, sizes, and types (based on file extensions).

## Usage

To use this utility, simply compile the source code provided in this repository using a compatible C compiler, and run the resulting executable with the folder path as a command-line argument. For example:
$ ./file_info_utility /path/to/folder


This will initiate the program to analyze the specified folder and provide a detailed summary of the contents.
Please note that this README is a high-level overview. For a more in-depth understanding, please refer to the source code comments and documentation.
