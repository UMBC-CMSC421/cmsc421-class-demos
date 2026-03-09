/*
 * Chapter 3: Processes
 * Demo 06: Zombie process
 *
 * What this demo shows:
 * - A terminated child remains as a zombie until the parent reaps it.
 * - The parent deliberately sleeps before calling waitpid().
 *
 * How to observe:
 * 1. Run this program in one terminal.
 * 2. In another terminal, run:
 *      ps -o pid,ppid,stat,cmd | grep 06_zombie_demo
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
        printf("[child ] exiting immediately; parent PID=%ld\n", (long)getppid());
        _exit(42);
    }

    printf("[parent] child PID=%ld created\n", (long)pid);
    printf("[parent] sleeping for 10 seconds before waitpid(); observe the zombie now\n");
    fflush(stdout);
    sleep(10);

    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        printf("[parent] reaped child; exit status=%d\n", WEXITSTATUS(status));
    }

    return 0;
}
