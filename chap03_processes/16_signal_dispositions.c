/*
 * Chapter 3: Processes
 * Demo 16: What each signal does if nobody handles it
 *
 * What's going on
 *   The kill command doesn't always kill. It sends a signal, and what happens next
 *   depends on the signal (Fox 4.6.2). If a process hasn't installed a handler, each
 *   signal has a default action, and there are only five of those:
 *     terminate    the process ends
 *     core         the process ends and can leave a core dump behind (demo 18)
 *     stop         the process freezes (state T) until it gets SIGCONT
 *     continue     a stopped process starts running again
 *     ignore       nothing happens at all
 *   SIGKILL and SIGSTOP are special. A program can't catch or ignore them, so they
 *   always work.
 *
 *   For every signal in the table below, this program starts a fresh child that
 *   just waits, sends it that one signal, and uses waitpid() to see what happened.
 *
 *   One more thing: when a process is killed by signal N, your shell sets $? to
 *   128 + N. That's how you tell "killed by a signal" apart from "exited with an error".
 *
 * Build
 *   make
 *
 * Run
 *   ./16_signal_dispositions         takes a few seconds
 *
 * What to look for
 *   The "default" column is what the man page says should happen. The last column is
 *   what actually happened. They should agree on every row.
 *
 * Other ways to see it
 *   kill -l                     all 64 signal names and numbers
 *   kill -l 143                 turns an exit status back into a name: TERM (143 = 128 + 15)
 *   sleep 100 &                 then: kill -TERM $!   then: wait $!   then: echo $?
 *                               prints 143
 *   sleep 100                   press Ctrl-C, then: echo $?     prints 130 (SIGINT is 2)
 *   man 7 signal                the full table this program checks
 */
#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* The table from man 7 signal: each signal and its default action. */
struct signal_row {
    int number;
    const char *default_action;
};

static const struct signal_row rows[] = {
    {SIGHUP, "terminate"},  {SIGINT, "terminate"},  {SIGQUIT, "core"},     {SIGILL, "core"},
    {SIGTRAP, "core"},      {SIGABRT, "core"},      {SIGBUS, "core"},      {SIGFPE, "core"},
    {SIGKILL, "terminate"}, {SIGUSR1, "terminate"}, {SIGSEGV, "core"},     {SIGUSR2, "terminate"},
    {SIGPIPE, "terminate"}, {SIGALRM, "terminate"}, {SIGTERM, "terminate"}, {SIGCHLD, "ignore"},
    {SIGCONT, "continue"},  {SIGSTOP, "stop"},      {SIGTSTP, "stop"},     {SIGTTIN, "stop"},
    {SIGURG, "ignore"},     {SIGWINCH, "ignore"},
};

static void short_sleep(void)
{
    struct timespec fifty_ms = {.tv_sec = 0, .tv_nsec = 50L * 1000L * 1000L};
    (void)nanosleep(&fifty_ms, NULL);
}

/* Start a child that just waits, send it one signal, and describe what happened. */
static void observe(int signo, char *result, size_t size)
{
    pid_t child = fork();
    pid_t changed = 0;
    int status = 0;

    if (child < 0) {
        /*
         * Never carry on with child == -1. kill(-1, signo) means "send this to every
         * process I'm allowed to signal", and that would take down your whole login.
         */
        (void)snprintf(result, size, "fork failed, skipped");
        return;
    }

    if (child == 0) {
        /*
         * Give the child its own process group. The kernel quietly throws away
         * SIGTSTP, SIGTTIN, and SIGTTOU sent to an "orphaned" group, which is what you
         * get when you start things over plain ssh. In its own group, the child's
         * parent (this program) is in the same session, so the stop signals act the
         * same no matter how you launched the demo.
         */
        (void)setpgid(0, 0);
        for (;;) {
            pause(); /* sleep until a signal shows up */
        }
    }

    short_sleep(); /* give the child a moment to get to pause() */
    (void)kill(child, signo);

    /*
     * Check up to 10 times, 50 ms apart. WNOHANG means "don't block if nothing has
     * happened yet". WUNTRACED means "also tell me if the child stopped".
     */
    for (int attempt = 0; attempt < 10 && changed == 0; attempt++) {
        changed = waitpid(child, &status, WNOHANG | WUNTRACED);
        if (changed == 0) {
            short_sleep();
        }
    }

    if (changed == 0) {
        (void)snprintf(result, size, "still running (nothing happened)");
    } else if (WIFSIGNALED(status)) {
        (void)snprintf(result, size, "terminated%s, shell $? = %d",
                       WCOREDUMP(status) ? " + core file" : "", 128 + WTERMSIG(status));
    } else if (WIFSTOPPED(status)) {
        (void)snprintf(result, size, "stopped (state T) until SIGCONT");
    } else if (WIFEXITED(status)) {
        (void)snprintf(result, size, "exited with status %d", WEXITSTATUS(status));
    }

    /* If the child is still around (ignored the signal, or got stopped), finish it off. */
    if (changed == 0 || WIFSTOPPED(status)) {
        (void)kill(child, SIGKILL);
        (void)waitpid(child, NULL, 0);
    }
}

int main(void)
{
    char result[96];

    printf("Each row: a fresh child that only calls pause(), then kill(child, signal).\n\n");
    printf("  %-8s %2s  %-10s %s\n", "signal", "#", "default", "what waitpid() observed");
    for (size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); i++) {
        observe(rows[i].number, result, sizeof(result));
        /* sigabbrev_np() turns a number into its short name, like 15 -> "TERM". */
        printf("  SIG%-5s %2d  %-10s %s\n",
               sigabbrev_np(rows[i].number), rows[i].number, rows[i].default_action, result);
        fflush(stdout);
    }

    printf("\n\"core\" signals only leave a core file when ulimit -c allows it (see demo 18).\n");
    printf("SIGCONT has no visible effect on a process that is already running.\n");
    return 0;
}
