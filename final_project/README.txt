The following document outlines instructions to run the various tasks and the files changed for the implementation.

Guide for Running the strace commands - 

Please note that all these commands are to be executed in the xv6 kernel.

Task 4.2.1 - 

Run "strace on" and enter commands such as "echo hi", "ls" and so on to see detailed tracing.

Task 4.2.2 - 

Run "strace off" and enter the same commands mentioned above and you can see commands run normally without tracing.

Task 4.2.3 - 

Run "strace run <command>" and see the detailed trace logs without turning strace on/off.

Task 4.2.4 - 

Once you run all commands with strace on, turn off strace and run strace dump. You can see the latest 
10 events in the dump. Change the STRACE_BUFFER_SIZE in proc.h to view more/less logs.

Task 4.2.5 - 

Run traceChild before turning strace on to see it run normally. Turn strace on and execute traceChild
to see the detailed strace logs of the program. 

You can also execute "strace run traceChild" for the same result.

Task 4.2.6 - 

In all the above commands, you can see the command outputs are suppressed. Execute abcd to see that error
logs are also suppressed with the exception of the exec log itself.

Task 4.4 - 

Once you run the commands in Task 4.2.4 and generate sufficient events, turn off strace and run
strace -o <filename>. Then open the file by running "cat <filename>" to view the dump.

Task 4.5 - 

In the Linux Kernel, there is a file straceReadLinux.c. Compile that by running "gcc -o straceReadLinux straceReadLinux.c".
Then run "strace ./straceReadLinux" to view the Linux strace logs. 

In the xv6 kernel, run "strace on" and then run "straceReadxv6" to view the xv6 strace logs for the program. You can also execute
"strace run straceReadxv6" for the same result.

Files Changed - 

1) Makefile - Added strace, traceChild and straceReadxv6 to UPROGS
2) proc.c - Added circular buffer for storing strace dump records and child process execution
3) proc.h - Added various parameters and structures for tracing events and adding flags.
4) strace.c - Created a user space program to handle strace functionality/
5) straceReadLinux - The executable to be run with strace to demonstrate Linux strace
6) straceReadLinux.c - The program for the above executable.
7) straceReadxv6.c - The program to demonstrate the use of strace in the xv6 kernel.
8) syscall.c - Added all the functionality for handling syscall tracing along with system calls added.
9) syscall.h - Added syscall numbers for the new system calls introduced.
10) sysproc.c - Added logic for the system calls added.
11) traceChild.c - The program to demonstrate how tracing is done for child processes.
12) user.h - Added function prototypes for the new system calls.
13) usys.S - Added signatures for the new system calls.