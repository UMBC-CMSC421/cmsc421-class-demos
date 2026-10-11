/*
 * Chapter 3: Processes
 * Demo 05: After fork(), parent and child have separate memory
 *
 * What's going on
 *   The child starts with a copy of every variable the parent had: same names, same
 *   values, even the same addresses. But they're separate copies. When the child
 *   changes its counters, the parent's counters don't move.
 *
 *   About the name "copy-on-write" (COW): Linux doesn't actually copy all of the
 *   memory at fork() time, because that would be slow. At first parent and child
 *   share the same physical pages, marked read-only. When one of them writes to a
 *   page, the kernel quietly makes a private copy of just that page. The program
 *   can't see that happen directly, but on Linux it can ask the kernel for a count.
 *   Before forking, the parent fills a 64 MB buffer. The child then reads
 *   /proc/self/smaps_rollup twice, and each time it prints two numbers:
 *     Shared_Dirty    memory this process has written that another process still maps
 *     Private_Dirty   memory only this process maps (its own copies)
 *
 * Build
 *   make
 *
 * Run
 *   ./05_fork_cow
 *
 * What to look for
 *   Part 1 (Linux only): right after fork, about 64 MB is shared and very little is
 *   private, so fork copied nothing. After the child writes to the buffer, those 64 MB
 *   have moved from shared to private: the kernel copied each page at the moment of
 *   the write.
 *   Part 2: the child prints 101 and 201. The parent, which prints after the child is
 *   done, still has 100 and 200. The addresses are identical in both. Those are virtual
 *   addresses. Each process has its own mapping from virtual addresses to real RAM,
 *   so the same address can point at different memory in different processes.
 *
 * Other ways to see it
 *   This one is over quickly, so outside tools don't catch much. Try editing it
 *   instead, then run make again:
 *   - Add a printf in the child BEFORE the += 1 lines. It shows 100 and 200, so the
 *     child really did start with the parent's values.
 *   - Move the two += 1 lines out of the child and into the parent, after waitpid().
 *     Now the parent's numbers change and the child's don't.
 *   - Change the memset in the child to write only half the buffer. Only about 32 MB
 *     moves from shared to private, because only the pages you touch get copied.
 */
#include <stdio.h>     /* printf(), perror(), fopen(), fgets(), sscanf() */
#include <stdlib.h>    /* malloc(), free() */
#include <string.h>    /* memset() */
#include <sys/types.h> /* pid_t */
#include <sys/wait.h>  /* waitpid() */
#include <unistd.h>    /* fork(), _exit() */

/* UL marks each number as an unsigned long, a type meant for big byte counts. */
#define BUFFER_BYTES (64UL * 1024UL * 1024UL) /* 64 MB */

static int global_counter = 100; /* a global, in the data area */

/*
 * #ifdef __linux__ ... #endif: the code between them only gets compiled on Linux,
 * because /proc/self/smaps_rollup only exists there.
 */
#ifdef __linux__
/*
 * Print this process's Shared_Dirty and Private_Dirty totals, in MB. The kernel adds
 * them up over every memory region when you read /proc/self/smaps_rollup.
 * "static" on a function means only this file can call it. moment is the label
 * printed at the start of the line.
 */
static void show_sharing(const char *moment)
{
    char line[256];
    long shared_kb = -1;  /* -1 means "not found yet" */
    long private_kb = -1;
    FILE *rollup = fopen("/proc/self/smaps_rollup", "r");

    /* fopen() returns NULL if it can't open the file. perror() prints why. */
    if (rollup == NULL) {
        perror("fopen /proc/self/smaps_rollup");
        return;
    }
    /*
     * fgets() reads one line at a time (at most sizeof(line) bytes) and returns NULL
     * at the end of the file. sscanf() tries to match each line against a pattern,
     * and only fills in the number when the line really is the Shared_Dirty or
     * Private_Dirty line. The (void) means we're ignoring its return value on purpose.
     */
    while (fgets(line, sizeof(line), rollup) != NULL) {
        (void)sscanf(line, "Shared_Dirty: %ld kB", &shared_kb);
        (void)sscanf(line, "Private_Dirty: %ld kB", &private_kb);
    }
    (void)fclose(rollup);

    /* The file reports kB, so divide by 1024 for MB. %-30s pads the label to line up. */
    printf("[child ] %-30s shared %3ld MB   private %3ld MB\n", moment, shared_kb / 1024,
           private_kb / 1024);
}
#endif

int main(void)
{
    int local_counter = 200; /* a local, on the stack */

    /*
     * Fill a big buffer BEFORE fork, so there's something large for the two processes
     * to share. malloc() gets the memory, and memset() writes 'P' into every byte.
     */
    char *buffer = malloc(BUFFER_BYTES);
    if (buffer == NULL) {
        perror("malloc");
        return 1;
    }
    memset(buffer, 'P', BUFFER_BYTES);

    /*
     * fork() makes the child. pid_t is the integer type for process IDs. The child
     * gets 0 back, the parent gets the child's PID, and -1 means it failed.
     */
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
#ifdef __linux__
        /* Part 1: nothing has been copied yet. The buffer is still the parent's pages. */
        show_sharing("right after fork:");

        /*
         * Write to every page of the buffer. Each first write to a page makes the kernel
         * copy that page.
         */
        memset(buffer, 'C', BUFFER_BYTES);
        show_sharing("after writing all 64 MB:");
        printf("\n");
#endif

        /*
         * Part 2: change our copies of the counters. This is the "write" in
         * copy-on-write.
         */
        global_counter += 1;
        local_counter += 1;

        /* %p prints an address. &x means "the address of x", cast to (void *) for %p. */
        printf("[child ] global_counter=%d at %p\n", global_counter, (void *)&global_counter);
        printf("[child ] local_counter =%d at %p\n", local_counter, (void *)&local_counter);
        fflush(stdout); /* push out anything printf is still holding */
        _exit(0); /* _exit instead of exit: see the comment in demo 04 */
    }

    /*
     * Parent: wait until the child has changed its values and exited, then print ours.
     * NULL means we don't need to know how the child ended. 0 means no special options.
     */
    if (waitpid(pid, NULL, 0) < 0) {
        perror("waitpid");
        return 1;
    }

    printf("[parent] global_counter=%d at %p\n", global_counter, (void *)&global_counter);
    printf("[parent] local_counter =%d at %p\n", local_counter, (void *)&local_counter);
    printf("[parent] buffer still starts with '%c' (the child wrote 'C' into its own copy)\n",
           buffer[0]);

    free(buffer);
    return 0;
}
