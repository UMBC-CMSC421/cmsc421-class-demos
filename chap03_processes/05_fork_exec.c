/*
 * Chapter 3: Processes
 * Demo 05: fork() and exec()
 *
 * What this demo shows:
 * - fork() duplicates the process.
 * - exec() replaces the child's program image with a new program.
 * - The parent can wait for the child and inspect its exit status.
 *
 * Suggested companion:
 *   strace -f ./05_fork_exec
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
        char *const argv[] = {"echo", "Hello from exec()", NULL};

        printf("[child ] about to replace process image with /bin/echo\n");
        fflush(stdout);
        execv("/bin/echo", argv);

        perror("execv");
        _exit(127);
    }

    int status = 0;

    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        printf("[parent] child exited with status %d\n", WEXITSTATUS(status));
    } else {
        printf("[parent] child did not exit normally\n");
    }

    return 0;
}
