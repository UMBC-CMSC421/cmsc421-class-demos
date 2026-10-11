/*
 * Chapter 3: Processes
 * Demo 11: Foreground, background, and job control
 *
 * What's going on
 *   A terminal lets exactly one process group be in the foreground at a time. That
 *   group gets what you type and gets your Ctrl-C. Anything else you started from
 *   that terminal is in the background. The shell moves jobs back and forth
 *   (Fox 4.3.2):
 *     Ctrl-Z    sends SIGTSTP, which stops (pauses) the foreground job
 *     bg        lets a stopped job keep running, but in the background
 *     fg        brings a job back to the foreground
 *     jobs      lists the jobs this shell knows about
 *   bg and fg both wake the job up by sending it SIGCONT.
 *   Only the foreground is allowed to read from the keyboard. A background job that
 *   tries gets stopped with SIGTTIN.
 *
 *   This program prints a line every second for 2 minutes. Each line says whether
 *   the program is in the foreground or the background at that moment.
 *
 * Build
 *   make
 *
 * Run (in a real terminal window, not piped into another command)
 *   ./11_foreground_background
 *       press Ctrl-Z, then type: jobs, bg, jobs, fg. Press Ctrl-C to end it.
 *   ./11_foreground_background &
 *       starts straight in the background. Type fg to bring it forward.
 *   ./11_foreground_background --read &
 *       a background job that wants to read. The shell says "Stopped (tty input)".
 *       Type fg, then type a line and press Enter.
 *
 * What to look for
 *   After Ctrl-Z the ticks stop and the shell prints "Stopped". After bg the ticks come
 *   back, the lines say "background", and you get a "SIGCONT arrived" line. Your prompt
 *   works again even while it keeps printing. After fg it says FOREGROUND again.
 *   Ctrl-C only reaches the foreground. If the job is in the background, fg it first,
 *   or use kill %1.
 *
 * Other ways to see it (in a second terminal)
 *   ps -o pid,pgid,tpgid,stat,cmd -C 11_foreground_background
 *       TPGID is the terminal's foreground group right now. When it equals the
 *       program's PGID, the program is in the foreground and STAT has a + in it.
 *       Run it before and after bg.
 *   kill -TSTP <PID>      does the same thing as Ctrl-Z
 *   kill -CONT <PID>      wakes it back up, but leaves it in the background
 */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/*
 * A flag the signal handler sets and the main loop checks. sig_atomic_t is a type that
 * can be written in one step, so a signal can't catch it half-written. volatile tells
 * the compiler to really read it from memory every time, because a handler can change
 * it behind the loop's back. static means it's only used in this file.
 */
static volatile sig_atomic_t continued = 0;

/*
 * Runs when SIGCONT arrives, which happens every time bg or fg wakes us up. The kernel
 * calls it in the middle of whatever main was doing, so it only sets the flag.
 */
static void handle_sigcont(int signo)
{
    (void)signo; /* we don't use signo. (void) tells the compiler that's on purpose. */
    continued = 1;
}

/*
 * tcgetpgrp() asks the terminal which process group is in the foreground right now.
 * If that's our group, we're the foreground job. STDIN_FILENO is file descriptor 0,
 * standard input, which is the terminal when you run this from a shell.
 */
static const char *job_position(void)
{
    pid_t terminal_group = tcgetpgrp(STDIN_FILENO);

    /* -1 means stdin isn't a terminal at all (say, input piped in from a file). */

    if (terminal_group < 0) {
        return "not attached to a terminal";
    }
    /* getpgrp() is our own group. a ? b : c gives b when a is true, c when it's false. */
    return terminal_group == getpgrp() ? "FOREGROUND" : "background";
}

int main(int argc, char *argv[])
{
    struct sigaction sa;
    /* 1 if you ran it with --read, 0 if not. strcmp returns 0 when two strings match. */
    int read_terminal = (argc == 2 && strcmp(argv[1], "--read") == 0);

    /*
     * Install the SIGCONT handler. memset zeroes the whole struct first so no field is
     * left holding garbage. sa_handler is the function to run. sa_mask lists signals to
     * hold off while the handler runs (none here). SA_RESTART means an interrupted read
     * just keeps going.
     */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigcont;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCONT, &sa, NULL) < 0) {
        perror("sigaction SIGCONT");
        return 1;
    }

    /*
     * Our process ID, process group ID, and session ID. The (long) casts make each
     * value match %ld. fflush pushes the text out now instead of letting it sit in
     * printf's buffer.
     */
    printf("PID=%ld  PGID=%ld  SID=%ld\n", (long)getpid(), (long)getpgrp(), (long)getsid(0));
    fflush(stdout);

    if (read_terminal) {
        char line[64];

        /* If we're in the background, this read gets us stopped with SIGTTIN until you fg us. */
        printf("[%s] about to read a line from the terminal\n", job_position());
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) != NULL) {
            printf("read: %s", line);
        }
        return 0;
    }

    /* 120 ticks, one per second, is the 2 minutes from the header. */
    for (int tick = 1; tick <= 120; tick++) {
        /* Fill in the struct by field name: 1 second and 0 nanoseconds. */
        struct timespec remaining = {.tv_sec = 1, .tv_nsec = 0};

        /* The handler set the flag. Report it here, then clear it for next time. */
        if (continued) {
            printf("  SIGCONT arrived: this job was stopped and has resumed\n");
            continued = 0;
        }

        printf("tick %3d  terminal's foreground PGID=%ld  this job is %s\n",
               tick, (long)tcgetpgrp(STDIN_FILENO), job_position());
        fflush(stdout);

        /*
         * Sleep one second. If a signal handler runs in the middle, nanosleep returns
         * early with EINTR and puts the time that was left into remaining. Loop until
         * the whole second is used up.
         */
        while (nanosleep(&remaining, &remaining) < 0 && errno == EINTR) {
            continue;
        }
    }

    return 0;
}
