#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <errno.h>


int main(int argc, char *argv[]) {
    //command line arguments, make sure one was given
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        return 1;
    }

    //creates shared memory region
    
    char shm_time[64];
    snprintf(shm_time, sizeof(shm_time), "/start_time_shm_%ld", (long)getpid());
    int sharedMem = shm_open(shm_time, O_CREAT | O_EXCL | O_RDWR, 0600);

    //check if it's actually open
    if (sharedMem == -1) {
        perror("shm_open");
        return 1;
    }

    // Make the shared-memory object large enough for struct timeval
    //error if that fails
    if (ftruncate(sharedMem, sizeof(struct timeval)) == -1) {
        perror("ftruncate");
        close(sharedMem);
        shm_unlink(shm_time);
        return 1;
    }

    // Map shared memory into address space.
    struct timeval *start_time = mmap(
        NULL,
        sizeof(struct timeval),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        sharedMem,
        0
    );
    
    //check if memory mapped properly
    if (start_time == MAP_FAILED) {
        perror("mmap");
        close(sharedMem);
        shm_unlink(shm_time);
        return 1;
    }

    //fork(), create child process
    // The mapping stays usable after its file descriptor is closed.
    close(sharedMem);
    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        munmap(start_time, sizeof(*start_time));
        shm_unlink(shm_time);
        return 1;
    }

    //gettimeofday() in child process
    //store starting struct timeval in shared memory region
    if (child == 0) {
        if (gettimeofday(start_time, NULL) == -1) {
            perror("gettimeofday");
            _exit(1);
        }

    //child uses execvp() to execute command from command line
        execvp(argv[1], &argv[1]);
        // A successful execvp does not return.
        perror("execvp");
        _exit(127);
    }

    //parent waits for child to terminate
    int status;
    while (waitpid(child, &status, 0) == -1) {
        if (errno == EINTR) {
            continue;
        }
        perror("waitpid");
        munmap(start_time, sizeof(*start_time));
        shm_unlink(shm_time);
        return 1;
    }

    //parent calls gettimeofday() to get end timestamp
    struct timeval end_time;
    if (gettimeofday(&end_time, NULL) == -1){
        perror("gettimeofday");
        munmap(start_time, sizeof(struct timeval));
        shm_unlink(shm_time);
        return 1;
    }

    //calculates elapsed time
    double startInSeconds = start_time->tv_sec + start_time->tv_usec / 1000000.0;
    double endInSeconds = end_time.tv_sec + end_time.tv_usec / 1000000.0;
    double elapsedTime = endInSeconds - startInSeconds;

    //print elapsed time in seconds, 6 digits after decimal point
    printf("Elapsed time: %.6f seconds\n", elapsedTime);

    //clean up shared memory resource then exit
    int cleanup_failed = 0;
    if (munmap(start_time, sizeof(*start_time)) == -1) {
        perror("munmap");
        cleanup_failed = 1;
    }
    if (shm_unlink(shm_time) == -1) {
        perror("shm_unlink");
        cleanup_failed = 1;
    }
    if (cleanup_failed) {
        return 1;
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}
