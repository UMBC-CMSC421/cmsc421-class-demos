/*
 * Chapter 3: Processes
 * Demo 22: Counting processes, and why output can double after fork()
 *
 * What's going on
 *   This is the "Lots of forks" program from the slides:
 *       fork(); fork(); printf("after two forks\n"); fork();
 *   Each fork() doubles the number of processes that run the code after it, so
 *   three forks in a row end with 2 x 2 x 2 = 8 processes, counting the original.
 *   The printf comes after two forks, so 4 processes run it.
 *
 *   The surprise is where printf's text goes. printf does not write right away. It
 *   collects text in a buffer inside the process and writes it out later:
 *     - to a terminal, the buffer is flushed at every newline (line buffering)
 *     - to a pipe or a file, it is flushed only when the buffer fills or the
 *       process exits (full buffering)
 *   When stdout is a pipe, the 4 processes still have "after two forks" sitting in
 *   their buffers when they call the third fork(). fork() copies the buffer along
 *   with everything else, so 8 processes each print the line when they exit.
 *
 * Build
 *   make
 *
 * Run
 *   ./22_fork_count_buffering                  to the terminal
 *   ./22_fork_count_buffering | wc -l          stdout into a pipe
 *   ./22_fork_count_buffering --flush | wc -l  same, with fflush() before fork 3
 *
 * What to look for
 *   To the terminal: 4 "after two forks" lines, plus 8 "process ... exiting" lines.
 *   The exiting lines go to stderr, which is never buffered, so they always show
 *   all 8 processes and wc does not count them.
 *   Piped: wc counts 8. With --flush: wc counts 4 again.
 *   The rule for Project 1: call fflush(stdout) before fork().
 *
 * Other ways to see it
 *   stdbuf -oL ./22_fork_count_buffering | wc -l    forces line buffering: 4
 *   strace -f -e trace=write ./22_fork_count_buffering | wc -l
 *       shows 8 write(1, ...) calls, one per process at exit, plus 8 writes to
 *       fd 2 for the stderr lines
 */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* argc is how many words were on the command line, and argv holds them. */
int main(int argc, char *argv[])
{
    /* 1 if you ran it with --flush, else 0. strcmp() returns 0 when two strings match. */
    int flush_before_third = (argc == 2 && strcmp(argv[1], "--flush") == 0);

    /*
     * (void) means we're ignoring fork()'s return value on purpose. Every process
     * runs the same code after each fork, so we don't care who is parent or child.
     */
    (void)fork(); /* 1 process becomes 2 */
    (void)fork(); /* 2 become 4 */
    printf("after two forks\n"); /* run by 4 processes */
    if (flush_before_third) {
        fflush(stdout); /* empty the buffer, so fork() has nothing to copy */
    }
    (void)fork(); /* 4 become 8, and each copy gets whatever is still in the buffer */

    /*
     * Each process waits for its own children, so the shell prompt comes back last.
     * wait() returns -1 once there are no children left, which ends the loop.
     * NULL means we don't need the exit status.
     */
    while (wait(NULL) > 0) {
    }

    /* stderr (file descriptor 2) isn't buffered, so this line goes out right away. */
    fprintf(stderr, "process %ld (parent %ld) exiting\n", (long)getpid(), (long)getppid());
    return 0; /* returning from main calls exit(), which flushes stdout */
}
