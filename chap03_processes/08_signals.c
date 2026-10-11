/*
 * Chapter 3: Processes
 * Demo 08: Catching signals
 *
 * What's going on
 *   A signal is a small notice the kernel delivers to a process, like "someone
 *   pressed Ctrl-C", "your child exited", or "please quit". Each one has a number
 *   and a name. If a process does nothing special, every signal has a default action.
 *   For SIGINT (Ctrl-C) the default is to kill the process. A process can also
 *   install a handler: a function that runs when that signal arrives.
 *   This program handles two signals:
 *     SIGINT    what Ctrl-C sends. Instead of dying we count it, and quit after 3.
 *     SIGUSR1   a signal with no built-in meaning, free for programs to use.
 *               You send it with the kill command.
 *
 * Build
 *   make
 *
 * Run
 *   ./08_signals                 it prints its PID, then waits
 *   Press Ctrl-C. It counts instead of quitting. The third Ctrl-C ends it.
 *   From a second terminal:  kill -USR1 <PID>
 *
 * What to look for
 *   Each signal makes the loop wake up and print the updated counts. Ctrl-C doesn't
 *   kill the program because our handler replaced the default action.
 *   Stuck? Ctrl-\ sends SIGQUIT. We don't handle that one, so it still quits.
 *
 * Other ways to see it (in the second terminal)
 *   kill -INT <PID>                  exactly what Ctrl-C does, just from outside
 *   grep SigCgt /proc/<PID>/status   the signals this process catches, as a hex
 *                                    bitmask. You'll see ...0202. Signal N is bit N-1,
 *                                    so 0x2 is SIGINT (2) and 0x200 is SIGUSR1 (10).
 *   strace -e trace=none -p <PID>    attaches to the running program and prints every
 *                                    signal as it arrives. When you use kill, si_pid
 *                                    is the PID of the shell you ran kill from.
 *                                    Ctrl-C the strace to detach.
 *   kill -l                          every signal name and number
 */
#include <signal.h> /* sigaction(), sigprocmask(), sigsuspend(), the sigset calls */
#include <stdio.h>  /* printf(), perror() */
#include <stdlib.h>
#include <unistd.h> /* getpid() */

/*
 * A handler can run at any moment, even in the middle of a line in main. So the
 * handlers do the least they can: bump a counter. sig_atomic_t is a type that's safe
 * to change inside a handler. volatile tells the compiler the value can change behind
 * main's back, so it has to re-read it every time instead of caching it.
 */
static volatile sig_atomic_t sigint_count = 0;
static volatile sig_atomic_t sigusr1_count = 0;

/*
 * The handlers. The kernel calls them for us when a signal arrives, and passes the
 * signal number as signo. "static" means only this file can see these names.
 */
static void handle_sigint(int signo)
{
    /* (void) marks signo as unused on purpose. We already know which signal it is. */
    (void)signo;
    sigint_count++;
}

static void handle_sigusr1(int signo)
{
    (void)signo;
    sigusr1_count++;
}

int main(void)
{
    struct sigaction sa_int;   /* settings for how to handle SIGINT */
    struct sigaction sa_usr1;  /* settings for how to handle SIGUSR1 */
    /* A sigset_t is a set of signals. These three get filled in below. */
    sigset_t blocked;
    sigset_t old_mask;
    sigset_t wait_mask;

    /*
     * Why block the signals first? Picture the simple version: check sigint_count,
     * then call pause() to sleep until a signal comes. If the signal lands in between
     * those two steps, the handler runs, and THEN pause() goes to sleep waiting for a
     * signal that already came and went. The program looks frozen.
     *
     * The fix: keep SIGINT and SIGUSR1 blocked (held back as "pending") the whole time,
     * and only let them through inside sigsuspend(). sigsuspend() unblocks them and
     * goes to sleep in one step that can't be split, so nothing slips through the gap.
     */
    /* Build the set {SIGINT, SIGUSR1}: start empty, then add each one. */
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGINT);
    sigaddset(&blocked, SIGUSR1);

    /*
     * SIG_BLOCK adds that set to what's already blocked. The old mask gets saved in
     * old_mask so we can put it back at the end. Like most system calls, it returns
     * -1 on failure, and perror() prints our label plus the reason.
     */
    if (sigprocmask(SIG_BLOCK, &blocked, &old_mask) < 0) {
        perror("sigprocmask block");
        return 1;
    }

    /*
     * The mask to use while sleeping: whatever was blocked before, minus our two
     * signals.
     */
    wait_mask = old_mask;
    sigdelset(&wait_mask, SIGINT);
    sigdelset(&wait_mask, SIGUSR1);

    /*
     * Fill in a struct sigaction for each signal:
     *   sa_handler   the function to run
     *   sa_mask      extra signals to hold back while the handler runs (none)
     *   sa_flags     special options (none)
     */
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;

    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;

    /*
     * Install the handlers. From now on these signals run our functions instead of
     * the default action.
     */
    if (sigaction(SIGINT, &sa_int, NULL) < 0) {
        perror("sigaction SIGINT");
        return 1;
    }

    if (sigaction(SIGUSR1, &sa_usr1, NULL) < 0) {
        perror("sigaction SIGUSR1");
        return 1;
    }

    printf("PID=%ld\n", (long)getpid());
    printf("Press Ctrl-C or send SIGUSR1 with: kill -USR1 %ld\n", (long)getpid());
    fflush(stdout);

    while (sigint_count < 3) {
        /*
         * Sleep with our two signals unblocked. Returns after a handler has run, and
         * then the two signals are blocked again.
         */
        (void)sigsuspend(&wait_mask);

        /* sig_atomic_t's exact type varies, so cast to int to match %d. */
        printf("Observed signals so far: SIGINT=%d, SIGUSR1=%d\n",
               (int)sigint_count,
               (int)sigusr1_count);
    }

    /* Put the signal mask back the way it was when we started. */
    if (sigprocmask(SIG_SETMASK, &old_mask, NULL) < 0) {
        perror("sigprocmask restore");
        return 1;
    }

    printf("Exiting after 3 SIGINT deliveries.\n");
    return 0;
}
