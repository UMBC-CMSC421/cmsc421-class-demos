/*
 * Chapter 13: File Systems
 * Lab program: memory-mapped file I/O with mmap().
 *
 * mmap() maps a file into the process's address space. After that, the
 * program changes the file with ordinary memory writes (here, strcpy)
 * instead of calling write(). The kernel moves the changed pages back to
 * the file.
 *
 * The program creates mapped.txt, sizes it to 4096 bytes, maps it, copies
 * one line of text into the mapping, and flushes it to the file.
 *
 * Build: make   (builds mmap_demo)
 * Run:   ./mmap_demo
 *        then: head -c 40 mapped.txt
 */

#define _XOPEN_SOURCE 700   /* exposes ftruncate and msync under -std=c11 */

#include <stdio.h>      /* printf, perror */
#include <fcntl.h>      /* open and its O_ flags */
#include <unistd.h>     /* ftruncate, close */
#include <sys/mman.h>   /* mmap, msync, munmap */
#include <string.h>     /* strcpy */
#include <stdlib.h>

#define FILE_SIZE 4096  /* one page on the course VM */

int main(void) {
    /* Step 1: create (or empty) mapped.txt and open it for read and write.
     * A shared, writable mapping needs the file opened with O_RDWR. */
    int fd = open("mapped.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    /* Step 2: set the file's length to 4096 bytes.
     * A new file has length 0, and a mapping cannot reach past the end of
     * the file, so we grow it first. The new bytes read as zeros. */
    if (ftruncate(fd, FILE_SIZE) < 0) {
        perror("ftruncate");
        close(fd);
        return 1;
    }

    /* Step 3: map the file into memory.
     * NULL           let the kernel pick the address
     * FILE_SIZE      map 4096 bytes
     * PROT_READ | PROT_WRITE   the program may read and write the memory
     * MAP_SHARED     writes go back to the file (MAP_PRIVATE would not)
     * fd, 0          map this file starting at offset 0
     * mmap() returns a pointer to the first byte, or MAP_FAILED on error. */
    char *map = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    /* Step 4: write to the file with a plain memory copy. No write() call. */
    strcpy(map, "Hello from memory-mapped file I/O.\n");

    /* Step 5: msync() with MS_SYNC asks the kernel to write the changed
     * pages to the file now and waits until it has. */
    if (msync(map, FILE_SIZE, MS_SYNC) < 0) {
        perror("msync");
        munmap(map, FILE_SIZE);
        close(fd);
        return 1;
    }

    /* Step 6: remove the mapping and close the file descriptor. */
    munmap(map, FILE_SIZE);
    close(fd);

    printf("Wrote to mapped.txt using memory access.\n");
    return 0;
}
