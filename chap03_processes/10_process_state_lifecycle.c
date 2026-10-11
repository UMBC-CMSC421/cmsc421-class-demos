/*
 * Chapter 3: Processes
 * Demo 10: Watching one process go through its states (Linux only)
 *
 * What's going on
 *   A process isn't just "running" or "done". This demo walks one child through four
 *   stages. At each one, the parent prints what Linux says about the child in the
 *   file /proc/<PID>/status:
 *     STOPPED    the child freezes itself with SIGSTOP, like Ctrl-Z      State: T
 *     SLEEPING   the parent sends SIGCONT, the child wakes up and naps   State: S
 *     ZOMBIE     the child has exited, the parent hasn't reaped it yet   State: Z
 *     REAPED     the parent calls waitpid() and the child is gone for good
 *   The pipe is only there so the child can tell the parent "I'm awake". Without it
 *   the parent might look before the child has actually started its nap.
 *
 * Build
 *   make
 *
 * Run
 *   ./10_process_state_lifecycle
 *   It reads /proc, so it needs Linux.
 *
 * What to look for
 *   The State: line in each block. Then compare the SLEEPING and ZOMBIE blocks: the
 *   zombie has no VmSize or VmRSS lines, because its memory is already freed. In the
 *   REAPED block the status file doesn't exist anymore.
 *   Name: says 10_process_stat, not the full name. The kernel keeps only the first
 *   15 characters of a process name.
 *
 * Other ways to see it
 *   In a second terminal start this first, then run the demo in the first terminal:
 *     watch -n 0.5 ps -o pid,ppid,stat,cmd -C 10_process_state_lifecycle
 *   watch reruns the ps twice a second (Ctrl-C quits it). You'll catch the child in S
 *   and then in Z. The stopped part goes by too fast to see.
 *   man 5 proc explains every line of the status file.
 */
#include <stdio.h>

/* Everything between #ifdef __linux__ and #else only gets compiled on Linux. */
#ifdef __linux__
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/*
 * Print a few lines from /proc/<pid>/status. That "file" isn't stored anywhere. The
 * kernel writes it fresh each time you read it, so it always shows the current state.
 */
static void print_status(pid_t pid, const char *phase)
{
    char path[64];
    char line[256];
    FILE *status;

    /*
     * Build the path, like "/proc/1234/status". snprintf is printf into a char array,
     * and sizeof(path) tells it how big the array is so it never writes past the end.
     * The (long) cast makes pid match %ld. The (void) means we're ignoring the return
     * value on purpose.
     */
    (void)snprintf(path, sizeof(path), "/proc/%ld/status", (long)pid);
    status = fopen(path, "r"); /* "r" means read. NULL means it couldn't open. */

    printf("\n[%s]\n", phase);
    if (status == NULL) {
        /*
         * After the child is reaped its /proc directory is gone. That's the point of the
         * last step. strerror(errno) turns the error code into words.
         */
        printf("%s is absent (%s)\n", path, strerror(errno));
        return;
    }

    /*
     * Go line by line and only keep the lines that start with one of these labels.
     * fgets reads one line and returns NULL at the end of the file. strncmp compares
     * just the first N characters and returns 0 when they match.
     */
    while (fgets(line, sizeof(line), status) != NULL) {
        if (strncmp(line, "Name:", 5) == 0 ||
            strncmp(line, "State:", 6) == 0 ||
            strncmp(line, "Pid:", 4) == 0 ||
            strncmp(line, "PPid:", 5) == 0 ||
            strncmp(line, "Threads:", 8) == 0 ||
            strncmp(line, "VmSize:", 7) == 0 ||
            strncmp(line, "VmRSS:", 6) == 0 ||
            strncmp(line, "voluntary_ctxt_switches:", 24) == 0 ||
            strncmp(line, "nonvoluntary_ctxt_switches:", 27) == 0) {
            fputs(line, stdout);
        }
    }

    (void)fclose(status);
}

int main(void)
{
    int ready_pipe[2]; /* [0] is the end you read from, [1] is the end you write to */
    int status = 0;

    /* pipe() fills in two file descriptors. Like most system calls, it returns -1 on error. */
    if (pipe(ready_pipe) < 0) {
        perror("pipe"); /* perror prints our label plus the reason it failed */
        return 1;
    }

    /*
     * fork() makes a copy of this process. It returns 0 in the child and the child's PID
     * in the parent. The copy includes both pipe ends, so the two processes can talk.
     */
    pid_t child = fork();
    if (child < 0) {
        perror("fork");
        return 1;
    }

    /* Only the child runs this block. */
    if (child == 0) {
        char ready = 'R';
        /* Fill in a struct by field name: 2 seconds and 0 nanoseconds. */
        struct timespec delay = {.tv_sec = 2, .tv_nsec = 0};

        (void)close(ready_pipe[0]); /* the child only writes, so close the read end */

        /*
         * Stop ourselves. raise() sends a signal to our own process. The child freezes
         * right here until someone sends SIGCONT.
         */
        raise(SIGSTOP);

        /*
         * We've been continued. Send one byte to tell the parent we're awake. write()
         * returns how many bytes it wrote, so anything but 1 means something went wrong.
         * _exit() ends the child right away. Plain exit() would also flush the stdio
         * buffers the child copied from the parent.
         */
        if (write(ready_pipe[1], &ready, 1) != 1) {
            _exit(1);
        }

        /* Nap for 2 seconds. While we sleep, our state is S. */
        (void)nanosleep(&delay, NULL);
        (void)close(ready_pipe[1]);
        _exit(23); /* the parent checks for 23 at the very end */
    }

    (void)close(ready_pipe[1]); /* the parent only reads, so close the write end */

    /*
     * Normally waitpid() only returns when a child exits. WUNTRACED makes it also
     * return when the child stops. That's how we know the SIGSTOP really happened.
     * waitpid() fills in status, and WIFSTOPPED(status) is true if the child stopped.
     */
    if (waitpid(child, &status, WUNTRACED) != child || !WIFSTOPPED(status)) {
        fprintf(stderr, "Did not observe the child stop as expected.\n");
        return 1;
    }
    print_status(child, "STOPPED: child raised SIGSTOP");

    /* Wake the child up. Despite its name, kill() just sends a signal, here SIGCONT. */
    if (kill(child, SIGCONT) < 0) {
        perror("kill SIGCONT");
        return 1;
    }

    /* read() blocks until the child writes its byte, so we don't look too early. */
    char ready;
    if (read(ready_pipe[0], &ready, 1) != 1) {
        fprintf(stderr, "Child did not report that it resumed.\n");
        return 1;
    }
    print_status(child, "SLEEPING: child resumed and called nanosleep");

    /*
     * The child naps 2 seconds and then exits. We wait 3. By then it has exited, but we
     * haven't called waitpid() yet, so it's a zombie.
     */
    struct timespec parent_delay = {.tv_sec = 3, .tv_nsec = 0};
    (void)nanosleep(&parent_delay, NULL);
    print_status(child, "ZOMBIE: child exited but has not been reaped");

    /* Reap it. This frees the child's last kernel record and its PID. */
    if (waitpid(child, &status, 0) != child) {
        perror("waitpid");
        return 1;
    }
    print_status(child, "REAPED: waitpid freed its last kernel record");

    /*
     * Double check the child ended the way we expected: a normal exit with code 23.
     * WIFEXITED says it exited normally, and WEXITSTATUS pulls out the exit code.
     */
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 23) {
        fprintf(stderr, "Unexpected child termination status.\n");
        return 1;
    }

    (void)close(ready_pipe[0]);
    return 0;
}
#else
/* Not Linux: there's no /proc to read, so just say so. */
int main(void)
{
    puts("This lifecycle-state demo requires Linux and /proc.");
    return 0;
}
#endif
