/*
 * Chapter 3: Processes
 * Demo 06: fork() then exec()
 *
 * What's going on
 *   This is how your shell runs every command you type. It takes two steps:
 *     fork()    make a copy of the current process (the child)
 *     exec()    the child throws out the program it was running and loads a
 *               different one, here /bin/echo. Same process, same PID, new program.
 *   exec does NOT create a process. It swaps the program inside one.
 *   If exec works it never returns, because the code that called it is gone. So any
 *   line after execv() only runs when exec failed.
 *   Meanwhile the parent waits for the child and checks how it ended.
 *
 * Build
 *   make
 *
 * Run
 *   ./06_fork_exec
 *
 * What to look for
 *   "Hello from exec()" is printed by the echo program, not by anything in this file.
 *   Then the parent reports exit status 0, which is what echo returns when it works.
 *
 * Other ways to see it
 *   strace -f -e trace=clone,execve,wait4 ./06_fork_exec
 *       clone (that's fork) returns the child's PID. Find the execve line for
 *       /bin/echo: it starts with [pid NNNN], and that's the same PID. No new process.
 *   Break it on purpose: change "/bin/echo" to "/bin/nope" and run make. Now execv
 *   fails, perror says why, and the parent sees exit status 127. That's the same
 *   number bash uses for "command not found".
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
        /*
         * The argument list for the new program. By convention argv[0] is the
         * program's own name, and the list has to end with NULL.
         */
        char *const argv[] = {"echo", "Hello from exec()", NULL};

        printf("[child ] about to replace process image with /bin/echo\n");

        /*
         * exec replaces ALL of this process's memory, including printf's output
         * buffer. Anything we haven't flushed yet would never get printed.
         */
        fflush(stdout);
        execv("/bin/echo", argv);

        /* We only get here if execv failed. */
        perror("execv");
        _exit(127);
    }

    int status = 0;

    /* The parent waits for the child. status gets filled in with how the child ended. */
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return 1;
    }

    /*
     * status packs several facts into one int. These macros pull them back out:
     *   WIFEXITED   did it exit on its own?      WEXITSTATUS  with what exit code?
     *   WIFSIGNALED was it killed by a signal?   WTERMSIG     which signal?
     */
    if (WIFEXITED(status)) {
        printf("[parent] child exited with status %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("[parent] child terminated by signal %d\n", WTERMSIG(status));
    } else {
        printf("[parent] child did not exit normally\n");
    }

    return 0;
}
