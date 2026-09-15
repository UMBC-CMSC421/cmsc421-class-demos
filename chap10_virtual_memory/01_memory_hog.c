/*
 * Chapter 10: Virtual Memory
 * Demo 01: use more and more memory until something stops it
 *
 * Allocates 10 MB at a time and writes to every byte, so the memory is really in use,
 * printing the running total. 02_memory_limits_oom.sh runs it under different limits.
 *
 * Run:
 *   ./01_memory_hog [max MB]        default 200
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CHUNK_MB 10

int main(int argc, char *argv[])
{
    long max_mb = argc > 1 ? strtol(argv[1], NULL, 10) : 200;
    const size_t chunk_bytes = (size_t)CHUNK_MB * 1024 * 1024;

    for (long used_mb = CHUNK_MB; used_mb <= max_mb; used_mb += CHUNK_MB) {
        char *block = malloc(chunk_bytes);

        if (block == NULL) {
            printf("malloc() returned NULL after %ld MB\n", used_mb - CHUNK_MB);
            return 1;
        }
        memset(block, 1, chunk_bytes); /* touching every page makes it count as used memory */
        printf("PID %ld is using %ld MB\n", (long)getpid(), used_mb);
        fflush(stdout);
    }

    printf("Reached %ld MB without being stopped\n", max_mb);
    return 0;
}
