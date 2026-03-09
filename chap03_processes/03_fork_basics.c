/*
 * Chapter 3: Processes
 * Demo 03: fork() basics
 *
 * What this demo shows:
 * - fork() creates a child process.
 * - Parent and child continue executing from the same source line after fork().
 * - The scheduler determines the exact output ordering.
 *
 * Try running this demo several times. The order of parent and child messages may vary.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        printf("[child ] PID=%ld PPID=%ld\n", (long)getpid(), (long)getppid());
        printf("[child ] fork() returned %ld in the child\n", (long)pid);
        _exit(0);
    }

    printf("[parent] PID=%ld created child PID=%ld\n", (long)getpid(), (long)pid);
    printf("[parent] fork() returned child's PID in the parent\n");

    if (waitpid(pid, NULL, 0) < 0) {
        perror("waitpid");
        return 1;
    }

    printf("[parent] child has been reaped\n");
    return 0;
}
