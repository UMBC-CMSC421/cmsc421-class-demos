/*
 * Chapter 3: Processes
 * Demo 25: How a shell builds  ls | wc -l
 *
 * What's going on
 *   This program does exactly what your shell does for  ls | wc -l :
 *     1. pipe()                    make the pipe
 *     2. fork() child 1            dup2(fd[1], 1): stdout now goes into the pipe
 *                                  close both pipe fds, then exec ls
 *     3. fork() child 2            dup2(fd[0], 0): stdin now comes from the pipe
 *                                  close both pipe fds, then exec wc -l
 *     4. parent                    close BOTH pipe ends, then wait for both children
 *   ls and wc never know about the pipe. ls writes to fd 1 and wc reads fd 0, as
 *   always. The two children are siblings; the parent created the pipe for them.
 *
 *   Quick reminder: every process starts with three open file descriptors.
 *   0 is stdin, 1 is stdout, and 2 is stderr. dup2() lets you swap what 0 or 1
 *   points to before exec, and the new program just uses them as usual.
 *
 *   With --forget-close the parent keeps its write end open for 3 seconds. wc can't
 *   finish until every write end is closed, so it waits even though ls is done.
 *
 * Build
 *   make
 *
 * Run
 *   ./25_pipeline_dup2
 *   ./25_pipeline_dup2 --forget-close
 *   ls | wc -l                           compare with the real shell
 *
 * What to look for
 *   The number matches what  ls | wc -l  prints. With --forget-close, the count
 *   only appears after the parent finally closes its write end.
 *
 * Other ways to see it
 *   strace -f -e trace=pipe2,dup2,execve,close ./25_pipeline_dup2
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    /* 1 if you ran it with --forget-close, 0 otherwise (strcmp returns 0 on a match). */
    int forget_close = (argc == 2 && strcmp(argv[1], "--forget-close") == 0);
    int fd[2];

    /* Step 1: make the pipe. fd[0] is the read end, fd[1] is the write end. */
    if (pipe(fd) < 0) {
        perror("pipe"); /* perror prints your text plus the reason the call failed */
        return 1;
    }
    fflush(stdout); /* empty stdout's buffer so the children don't inherit a copy */

    /* Step 2: the first child becomes ls, the left side of the pipe. */
    pid_t ls_pid = fork();
    if (ls_pid < 0) {
        perror("fork");
        return 1;
    }
    if (ls_pid == 0) {
        dup2(fd[1], STDOUT_FILENO); /* fd 1 now refers to the pipe's write end */
        close(fd[0]);
        close(fd[1]);               /* the copy on fd 1 is all ls needs */
        /* exec replaces this child's program with ls. The (char *)NULL marks the end
           of the argument list. If exec works, it never returns. */
        execlp("ls", "ls", (char *)NULL);
        /* Still here? Then exec failed. 127 is the shell's code for "command not found."
           _exit() leaves right away without flushing the parent's copied buffers. */
        perror("execlp ls");
        _exit(127);
    }

    /* Step 3: the second child becomes wc -l, the right side of the pipe. */
    pid_t wc_pid = fork();
    if (wc_pid < 0) {
        perror("fork");
        return 1;
    }
    if (wc_pid == 0) {
        dup2(fd[0], STDIN_FILENO);  /* fd 0 now refers to the pipe's read end */
        close(fd[0]);
        close(fd[1]);               /* if wc kept this, it would wait for itself */
        execlp("wc", "wc", "-l", (char *)NULL);
        perror("execlp wc");
        _exit(127);
    }

    /* Step 4: the parent. It doesn't read or write the pipe, so it closes both ends. */
    close(fd[0]);
    if (forget_close) {
        /* Hold on to the write end for a bit, so you can watch wc get stuck. */
        fprintf(stderr, "[parent] still holding the write end; wc has to wait...\n");
        sleep(3);
        fprintf(stderr, "[parent] closing it now\n");
    }
    close(fd[1]); /* without this, wc never sees EOF */

    /* Reap both children so neither one is left as a zombie. */
    int status = 0;
    waitpid(ls_pid, &status, 0);
    waitpid(wc_pid, &status, 0);
    return 0;
}
