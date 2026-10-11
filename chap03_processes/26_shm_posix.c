/*
 * Chapter 3: Processes
 * Demo 26: POSIX shared memory between two unrelated processes
 *
 * What's going on
 *   Shared memory takes a few system calls to set up, then none at all to use:
 *     shm_open("/name")   create or open a named shared-memory object
 *     ftruncate(fd, n)    set its size (producer only)
 *     mmap(...)           map it into this address space and get a pointer
 *     (use the pointer)   ordinary memory reads and writes, no kernel involved
 *     shm_unlink("/name") remove the name when everyone is done
 *   The producer writes a message into the object. The consumer, started later as a
 *   separate program, maps the same object and reads it. The two processes aren't
 *   related: the name is how they find each other.
 *   The consumer opens the object read-only, so it must map it with PROT_READ only.
 *
 * Build
 *   make
 *
 * Run
 *   ./26_shm_posix producer
 *   ls -l /dev/shm                       the object is there, size 4096
 *   ./26_shm_posix consumer              prints the message, then removes the object
 *   ls -l /dev/shm                       gone
 *
 * What to look for
 *   The consumer prints the producer's PID and message even though the producer has
 *   already exited. A shared-memory object lives in RAM until someone unlinks it or
 *   the machine reboots, much like a file.
 *
 * Other ways to see it
 *   cat /dev/shm/cmsc421_demo           the bytes are readable like a file
 *   strace -e trace=openat,ftruncate,mmap ./26_shm_posix producer
 */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

/* Both programs must use the same name. On Linux it shows up as /dev/shm/cmsc421_demo. */
#define SHM_NAME "/cmsc421_demo"
#define SHM_SIZE 4096

/* "static" here just means these functions are private to this file. */
static int producer(void)
{
    /* Create the object (O_CREAT) or open it if it's already there, for reading
       and writing (O_RDWR). 0666 is the permission bits, like chmod (your umask
       may remove some). Like open(), this returns a file descriptor, or -1 on failure. */
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        perror("shm_open"); /* perror prints your text plus the reason it failed */
        return 1;
    }
    /* A new object starts with size 0. Make it SHM_SIZE bytes. */
    if (ftruncate(fd, SHM_SIZE) < 0) {
        perror("ftruncate");
        return 1;
    }
    /* Map the object into our address space. NULL lets the kernel pick the address.
       MAP_SHARED means our writes go to the shared object, so others can see them.
       mmap() returns MAP_FAILED (not NULL) when it fails, so check for that. */
    char *ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    close(fd); /* the mapping stays valid after the fd is closed */

    /* From here on this is plain memory. No system call per write.
       snprintf() prints into ptr like printf() prints to the screen, and never
       writes more than SHM_SIZE bytes. The (long) cast matches the %ld format. */
    snprintf(ptr, SHM_SIZE, "Hello from producer PID %ld\n", (long)getpid());

    /* Drop our mapping. The object and its bytes stay in /dev/shm. */
    munmap(ptr, SHM_SIZE);
    printf("producer: wrote the message into %s and exited without unlinking it\n", SHM_NAME);
    return 0;
}

static int consumer(void)
{
    /* Open the existing object by name, read-only. No O_CREAT, so this fails if
       the producer hasn't run yet. The mode argument only matters when creating. */
    int fd = shm_open(SHM_NAME, O_RDONLY, 0);
    if (fd < 0) {
        perror("shm_open (run the producer first)");
        return 1;
    }
    /* Opened read-only, so map read-only. Asking for PROT_WRITE here would fail. */
    char *ptr = mmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    close(fd);

    /* Just read the memory. The producer's message is already sitting there. */
    printf("consumer read: %s", ptr);

    munmap(ptr, SHM_SIZE);
    shm_unlink(SHM_NAME); /* remove the name; the memory is freed once unmapped everywhere */
    printf("consumer: unlinked %s\n", SHM_NAME);
    return 0;
}

/* Pick a role from the command line. strcmp() returns 0 when the strings match. */
int main(int argc, char *argv[])
{
    if (argc == 2 && strcmp(argv[1], "producer") == 0) return producer();
    if (argc == 2 && strcmp(argv[1], "consumer") == 0) return consumer();
    fprintf(stderr, "Usage: %s producer|consumer\n", argv[0]);
    return 2;
}
