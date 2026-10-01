#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>


static void child_process(int fd[2], char *argv[]) 
{
    struct timeval start;

    close(fd[0]); // Close the unused read end of the pipe

    // In the child process, call gettimeofday() to record the starting timestamp.
    if(gettimeofday(&start, NULL) == -1)
    {
        perror("gettimeofday");
        close(fd[1]);
        _exit(EXIT_FAILURE);
    }

    // Write the starting struct timeval to the pipe
    if (write(fd[1], &start, sizeof(start)) != (ssize_t)sizeof(start)) 
    {
        perror("write");
        close(fd[1]);
        _exit(EXIT_FAILURE);
    }

    close(fd[1]); // Close the write end of the pipe now that we are done with it

    // The child uses execvp() to execute the command given on the command line. execvp only returns if it fails
    execvp(argv[1], &argv[1]); 
    fprintf(stderr, "execvp failed: %s\n", strerror(errno));
    _exit(127);
}

// TODO Parent Process
static int parent_process(int fd[2], pid_t pid)
{
    ; 
}

int main(int argc, char *argv[]) 
{
    int fd[2]; // fd[0] is for reading, fd[1] is for writing
    pid_t pid;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Create the pipe before fork() so both parent and child inherit the pipe descriptors
    if (pipe(fd) == -1)
    {
        perror("pipe");
        return EXIT_FAILURE;
    }

    // Call fork() to create the child processes
    pid = fork();
    if (pid < 0)
    {
        perror("fork");
        close(fd[0]);
        close(fd[1]);
        return EXIT_FAILURE;
    }

    if (pid == 0)
        child_process(fd, argv); // Child process
    
    return parent_process(fd, pid); // Parent process
}
