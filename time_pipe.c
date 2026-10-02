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

static int parent_process(int fd[2], pid_t pid)
{
    struct timeval start;
    struct timeval end;
    int status;

    // Parent does not write
    close(fd[1]);

    // Wait for child to finish, CORRECTION: retrying if wait interrupted by a signal @ee
    while (waitpid(pid, &status, 0) == -1)
    {
        if (errno == EINTR)
            continue;
        perror("waitpid");
        close(fd[0]);
        return EXIT_FAILURE;
    }

    // Record Time
    if (gettimeofday(&end, NULL) == -1)
    {
        perror("gettimeofday");
        close(fd[0]);
        return EXIT_FAILURE;
    }

    // Read the pipe, get time
    ssize_t bytes_read = read(
            fd[0],
            &start,
            sizeof(struct timeval)
    );

    // Close the read-end of pipe
    close(fd[0]);

    // CORRECTION: child must send complete timestamp through pipe @ee
    if (bytes_read != (ssize_t)sizeof(struct timeval))
    {
        if (bytes_read == -1)
            perror("read");
        else
            fprintf(stderr, "read: incomplete start timestamp (%zd of %zu bytes)\n",
                    bytes_read, sizeof(struct timeval));
        return EXIT_FAILURE;
    }

    // Add whole + fractional seconds to output recorded time
    double starttime =
        start.tv_sec + start.tv_usec / 1000000.0;

    double endtime =
        end.tv_sec + end.tv_usec / 1000000.0;

    double elapsedtime = endtime - starttime;

    printf("Elapsed time: %.6f seconds\n", elapsedtime);

    // pass command's exit status back to the shell @ee
    return WIFEXITED(status) ? WEXITSTATUS(status) : EXIT_FAILURE;
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
