#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/time.h> //has timeval needed later


int main(int argc, char *argv[]) {
    //command line arguments, make sure one was given


    //creates shared memory region
    const char *shm_time = "/start_time_shm";
    int sharedMem = shm_open(shm_time, O_CREAT | O_RDWR, 0600);

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

    //gettimeofday() in child process

    //store starting struct timeval in shared memory region

    //child uses execvp() to execute command from command line

    //parent waits for child to terminate

    //parent calls gettimeofday() to get end timestamp

    //parent reads start time from shared memory and calculates elapsed time

    //print elapsed time in seconds, 6 digits after decimal point

    //clean up shared memory resource then exit
}