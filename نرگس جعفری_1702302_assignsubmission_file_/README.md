In the name of God

OS Midterm Project , 1402
__________________________________________________________________________________

In this project, we aim to provide a simplified version of NCDU using
processes and threads. The program starts by traversing a directory
(provided by user input) using processes and threads and calculates the
required information in the problem (total number of files, types of
files, the size of the largest and smallest files, and the size of the
input directory). To calculate these metrics, we use the shared memory
technique, and each process or thread during traversal stores the
calculated information in a structure called FileInfo designed for this
purpose.

To prevent unintended changes in shared memory, semaphores are used for
processes, and mutex locks are used for threads to ensure that at any
given time, only one process or thread can enter the critical section.

How to run the program:

The program execution starts from the main function after receiving the
directory address from the user and checking its existence. The function
(*Traverse*) is called to initiate the traversal. Prior to this, a
semaphore is created for synchronization and management of the critical
section, initialized with a value of 1. In this function, if a directory
is identified, the parent process uses the fork function to create a
child for that directory, and then the function (*TraverseWithThread*)
is called to traverse it with a thread.

At the beginning of the function, a wait is called for the semaphore and
enters the full state (to prevent another process from entering the
critical section). Afterward, if a new directory is identified, a thread
is created and assigned to it. The (*ThreadFunc*) function is executed
by each thread, where the information related to the directory is
examined, and the values of the FileInfo structure (shared memory) are
updated.

At the beginning of this function, a mutex lock is used to manage the
critical section. The lock is acquired as soon as a thread enters, and
at the end of the process, after complete traversal and updating of the
required values, the lock is released. After finishing the work of a
process (completion of all threads of that process and complete
traversal of the directory), the sem_post function is called for the
semaphore, and the semaphore is in the empty state. The next process can
enter the critical section in the (*TraverseWithThraed*) function.

This process continues until all files are traversed, and finally, the
size of the input directory is calculated.

