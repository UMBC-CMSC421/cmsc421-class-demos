/*
 * Chapter 3: Processes
 * Demo 04: Copy-on-write intuition
 *
 * What this demo shows:
 * - Parent and child begin with the same virtual contents after fork().
 * - Once one process writes to its copy, the values diverge.
 * - The variable names are the same, but the processes do not share writable user-space memory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int global_counter = 100;

int main(void)
{
    int local_counter = 200;
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        global_counter += 1;
        local_counter += 1;

        printf("[child ] global_counter=%d at %p\n", global_counter, (void *)&global_counter);
        printf("[child ] local_counter =%d at %p\n", local_counter, (void *)&local_counter);
        _exit(0);
    }

    if (waitpid(pid, NULL, 0) < 0) {
        perror("waitpid");
        return 1;
    }

    printf("[parent] global_counter=%d at %p\n", global_counter, (void *)&global_counter);
    printf("[parent] local_counter =%d at %p\n", local_counter, (void *)&local_counter);

    return 0;
}
