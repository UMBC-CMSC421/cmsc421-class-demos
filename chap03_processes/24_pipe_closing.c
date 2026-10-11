/*
 * Chapter 3: Processes
 * Demo 24: pipe() after fork(), and the closing rule
 *
 * What's going on
 *   pipe(fd) gives one process two file descriptors: fd[0] to read, fd[1] to write.
 *   After fork() the child has copies of both, so there are four ends. Here the
 *   parent writes and the child reads, so the parent should close fd[0] and the
 *   child should close fd[1].
 *
 *   Why bother closing? read() returns 0 (end of file) only when EVERY write end is
 *   closed. And writing to a pipe that has no read end left raises SIGPIPE, which
 *   kills the writer by default.
 *
 *   Each mode below breaks one rule on purpose. These are the four cases on the
 *   "Closing Rule" slides:
 *     ok                  everyone closes what they don't use
 *     writer-keeps        case 1: the parent never closes its fd[1]
 *     reader-keeps        case 2: the child never closes its own fd[1]
 *     writer-keeps-read   case 3: the parent never closes its fd[0]
 *     reader-exits        case 4: the child exits early; the parent keeps writing
 *   That's five modes: ok plus one per case. To keep a hang from freezing your
 *   terminal, the child gives up after 3 seconds.
 *
 * Build
 *   make
 *
 * Run
 *   ./24_pipe_closing ok
 *   ./24_pipe_closing writer-keeps
 *   ./24_pipe_closing reader-keeps
 *   ./24_pipe_closing writer-keeps-read
 *   ./24_pipe_closing reader-exits; echo "exit status $?"
 *
 * What to look for
 *   ok and writer-keeps-read: the child prints the 3 lines, then "EOF". Case 3 has
 *   no visible effect.
 *   writer-keeps and reader-keeps: the child prints the 3 lines, then waits for an
 *   EOF that never comes. After 3 seconds it reports that a write end is still open.
 *   reader-exits: the parent dies on its first write. The shell shows exit status
 *   141 = 128 + 13, and 13 is SIGPIPE.
 *
 * Other ways to see it
 *   During a hang, from another terminal:  ls -l /proc/<child PID>/fd
 *   reader-keeps shows the child holding both pipe ends.
 *   yes | head -1; echo ${PIPESTATUS[0]}     yes dies of SIGPIPE too: 141
 */
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * The child runs this when its 3-second alarm goes off (SIGALRM).
 * "static" on a function just means it's private to this file.
 */
static void child_gives_up(int sig)
{
    (void)sig; /* (void) says "I know I'm not using this," so the compiler won't warn */
    /* A signal handler can interrupt the program anywhere, so it may only call
       async-signal-safe functions. write() and _exit() are safe; printf() is not. */
    static const char msg[] =
        "[child ] no EOF after 3 s: some process still holds a write end. Giving up.\n";
    /* sizeof(msg) counts the hidden '\0' at the end of the string, so subtract 1. */
    (void)write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    _exit(1);
}

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s ok|writer-keeps|reader-keeps|writer-keeps-read|reader-exits\n", prog);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        usage(argv[0]);
        return 2;
    }
    /* strcmp() returns 0 when two strings match, so each flag below is 1 or 0. */
    const char *mode = argv[1];
    int writer_keeps = strcmp(mode, "writer-keeps") == 0;
    int reader_keeps = strcmp(mode, "reader-keeps") == 0;
    int writer_keeps_read = strcmp(mode, "writer-keeps-read") == 0;
    int reader_exits = strcmp(mode, "reader-exits") == 0;
    if (!writer_keeps && !reader_keeps && !writer_keeps_read && !reader_exits && strcmp(mode, "ok") != 0) {
        usage(argv[0]);
        return 2;
    }

    /* Make the pipe. pipe() fills in fd[0] (read end) and fd[1] (write end).
       Like most system calls, it returns -1 on failure, and perror() prints why. */
    int fd[2];
    if (pipe(fd) < 0) {
        perror("pipe");
        return 1;
    }

    fflush(stdout); /* flush before fork, so nothing is printed twice */
    /* fork() makes a copy of this process. From here on, two processes run this code.
       fork() returns 0 in the child and the child's PID in the parent. */
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        /* ---------------- child: the reader ---------------- */
        if (!reader_keeps) {
            close(fd[1]); /* we never write, so close our copy of the write end */
        }
        if (reader_exits) {
            /* Case 4: close the read end and leave. Now the pipe has no reader. */
            close(fd[0]);
            printf("[child ] exiting without reading anything\n");
            fflush(stdout);
            /* _exit() ends the child right away, without running the parent's
               cleanup code that the child inherited from fork(). */
            _exit(0);
        }
        /* Set up the safety net: run child_gives_up() if we're still here in 3 s. */
        signal(SIGALRM, child_gives_up);
        alarm(3);

        /* fdopen wraps the read end in a FILE so we can read one line at a time.
           fgets() returns NULL when the underlying read() returns 0: end of file. */
        FILE *in = fdopen(fd[0], "r");
        char line[128];
        while (in != NULL && fgets(line, sizeof(line), in) != NULL) {
            printf("[child ] read: %s", line);
            fflush(stdout);
        }
        /* We only get here once read() said "no more data, ever." */
        printf("[child ] read() returned 0: EOF, every write end is closed\n");
        fflush(stdout);
        _exit(0);
    }

    /* ---------------- parent: the writer ---------------- */
    if (!writer_keeps_read) {
        close(fd[0]); /* we never read, so close our copy of the read end */
    }
    if (reader_exits) {
        /* Wait until the child has exited, so its read end is certainly closed. */
        waitpid(pid, NULL, 0);
        fprintf(stderr, "[parent] writing to a pipe with no reader left...\n");
    }

    /* Send three lines into the pipe. snprintf() builds the text in line[] and never
       writes past sizeof(line); it returns how many characters it wrote. */
    for (int i = 1; i <= 3; i++) {
        char line[64];
        int len = snprintf(line, sizeof(line), "line %d from the parent\n", i);
        /* write() wants a size_t (an unsigned size), so we cast len to (size_t). */
        if (write(fd[1], line, (size_t)len) < 0) {
            perror("write"); /* only reached if SIGPIPE is ignored: then errno is EPIPE */
            return 1;
        }
    }
    if (!writer_keeps) {
        close(fd[1]); /* done writing: this is what lets the child see EOF */
    }

    /* Wait for the child to finish and collect its exit status (no zombie left). */
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return 1;
    }
    /* WIFEXITED is true if the child exited normally (not killed by a signal).
       WEXITSTATUS then pulls out its exit code. */
    printf("[parent] child exited with status %d\n", WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    if (writer_keeps_read) {
        printf("[parent] case 3: an unused read end has no visible effect here\n");
    }
    return 0;
}
