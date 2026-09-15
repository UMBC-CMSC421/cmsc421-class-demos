/*
 * Chapter 3: Processes
 * Demo 04: fork() basics
 *
 * What's going on
 *   fork() makes a copy of the process that calls it. After that line there are two
 *   processes running this same program, and BOTH of them come back out of fork().
 *   The only difference between them is what fork() returned:
 *     -1     fork failed and there is no child
 *      0     you're the child
 *     > 0    you're the parent, and the number is the new child's PID
 *   The if statements below are how each process works out which one it is.
 *
 * Build
 *   make
 *
 * Run
 *   ./04_fork_basics
 *
 * What to look for
 *   The child's PPID is the parent's PID. fork() gave 0 to the child and gave the
 *   child's PID to the parent. Now run it several times. Nothing in the code decides
 *   who prints first. The scheduler does, so don't count on the order.
 *
 * Other ways to see it
 *   strace -f -e trace=clone,wait4 ./04_fork_basics
 *       strace prints the system calls a program makes. -f follows the child too.
 *       On Linux, fork() is really a system call named clone, and you can see it
 *       return the child's PID. wait4 is what waitpid() turns into.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    pid_t pid = fork();

    /* From this line on, two processes are running this code. */

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        /* Only the child gets in here. */
        printf("[child ] PID=%ld PPID=%ld\n", (long)getpid(), (long)getppid());
        printf("[child ] fork() returned %ld in the child\n", (long)pid);

        /*
         * fflush() pushes out anything printf is still holding onto. Then _exit() (with
         * the underscore) ends the child immediately, skipping the usual cleanup.
         * Why not plain exit()? exit() flushes printf's buffer, and the child got a copy
         * of the parent's buffer when it was forked. Anything the parent hadn't printed
         * yet could come out twice.
         */
        fflush(stdout);
        _exit(0);
    }

    /* Only the parent gets here. The child already called _exit(). */
    printf("[parent] PID=%ld created child PID=%ld\n", (long)getpid(), (long)pid);
    printf("[parent] fork() returned %ld in the parent\n", (long)pid);

    /*
     * Wait for this particular child to finish. After a child exits, it sticks around
     * as a zombie until its parent collects it like this (see demo 07). Collecting a
     * finished child is called "reaping" it.
     */
    if (waitpid(pid, NULL, 0) < 0) {
        perror("waitpid");
        return 1;
    }

    printf("[parent] child has been reaped\n");
    return 0;
}
