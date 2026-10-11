/*
 * Chapter 3: Processes
 * Demo 23: What wait() really returns
 *
 * What's going on
 *   waitpid(pid, &status, 0) fills in status, but status is not the exit code. It is
 *   one int that packs several facts, and you decode it with macros:
 *     WIFEXITED(status)    true if the child called exit() or returned from main
 *     WEXITSTATUS(status)  then: the exit code, 0 to 255
 *     WIFSIGNALED(status)  true if a signal killed the child
 *     WTERMSIG(status)     then: the signal number
 *   The shell does the same decoding for $?: an exit code is shown as is, and a
 *   death by signal N is shown as 128 + N.
 *
 *   This program starts three children: one calls exit(3), one is killed with
 *   SIGKILL, and one crashes with a segmentation fault (SIGSEGV).
 *
 * Build
 *   make
 *
 * Run
 *   ./23_wait_status
 *
 * What to look for
 *   The raw status for exit(3) is 768 (0x300) on Linux: the exit code sits in the
 *   second byte. The macros turn it back into 3. The killed children have small raw
 *   values (the signal number in the low bits) and the shell would report 137 and 139.
 *   If core dumps are enabled (ulimit -c above 0, or systemd-coredump), SIGSEGV also
 *   sets the core-dump bit 0x80, so its raw status becomes 139 (0x8b); WTERMSIG is
 *   still 11.
 *
 * Other ways to see it
 *   sh -c 'exit 3'; echo $?                         3
 *   sleep 100 & kill -KILL $!; wait $!; echo $?     137 (128 + 9)
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * Wait for one child, then print its status two ways: the raw int, and what the
 * macros pull out of it. static means only this file uses this helper.
 */
static void report(const char *what, pid_t pid)
{
    int status = 0;

    /*
     * Block until child pid ends. &status gives waitpid() the address of status, so
     * it can fill it in. A return value below 0 means the call failed, and perror()
     * prints "waitpid: " plus the reason.
     */
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        exit(1);
    }

    /*
     * %04x prints the same number in hex, which makes the bytes easy to see.
     * (unsigned) converts status to the type %x expects.
     */
    printf("%-28s raw status = %5d (0x%04x)   ", what, status, (unsigned)status);
    if (WIFEXITED(status)) {
        printf("WIFEXITED,   WEXITSTATUS = %3d   shell $? = %d\n", WEXITSTATUS(status),
               WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("WIFSIGNALED, WTERMSIG    = %3d   shell $? = %d\n", WTERMSIG(status),
               128 + WTERMSIG(status));
    }
}

int main(void)
{
    pid_t pid; /* pid_t is the type that holds a process ID */

    fflush(stdout); /* empty stdout's buffer, so the children don't inherit a copy */

    /*
     * Child 1: a normal exit with code 3.
     * fork() returns 0 in the child and the child's PID in the parent.
     * _exit() ends the child right away without flushing stdio buffers.
     */
    pid = fork();
    if (pid == 0) {
        _exit(3);
    }
    report("child called _exit(3):", pid);

    /* Child 2: sits in pause() until the parent kills it. */
    pid = fork();
    if (pid == 0) {
        pause();  /* sleep until a signal arrives */
        _exit(0); /* never reached: SIGKILL ends the child first */
    }
    /* (void): we're ignoring kill()'s return value on purpose. */
    (void)kill(pid, SIGKILL);
    report("child killed by SIGKILL:", pid);

    /* Child 3: gets SIGSEGV, the signal the kernel sends for a bad memory access. */
    pid = fork();
    if (pid == 0) {
        (void)raise(SIGSEGV); /* raise() sends a signal to yourself */
        _exit(0);             /* never reached */
    }
    report("child got SIGSEGV:", pid);

    return 0;
}
