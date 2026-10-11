/*
 * Chapter 3: Processes
 * Demo 14: Every process state letter, on demand
 *
 * What's going on
 *   ps and top show a one-letter state for each process (Fox Table 4.2). This program
 *   makes one child in each state, so you can see them all at once:
 *     R   running, or ready to run            a child stuck in a busy loop
 *     S   sleeping until something happens    a child waiting in pause()
 *     D   sleeping, and most signals can't    a parent waiting for its vfork()
 *         wake it (usually means waiting on   child to finish
 *         the disk)
 *     T   stopped                             a child that sent itself SIGSTOP
 *     t   stopped by a debugger               a child being traced with ptrace()
 *     Z   zombie                              a child that exited and wasn't reaped
 *   It reads each child's state straight from the kernel (/proc/<PID>/stat), prints a
 *   table, and then waits so you can look at the children yourself.
 *
 * Build
 *   make
 *
 * Run
 *   ./14_state_codes
 *   It prints a ps command. While it waits, paste that command into a second
 *   terminal. When you're done, press Enter (or Ctrl-C) in the first terminal, and it
 *   kills and reaps every child.
 *
 * What to look for
 *   The state column in the program's table matches the STAT column in ps. ps may add
 *   extra letters after the state, like + for the foreground group. WCHAN is the name
 *   of the kernel function each sleeping child is waiting inside.
 *
 * Other ways to see it (in the second terminal, with the PIDs it printed)
 *   grep State /proc/<PID>/status     the state spelled out, like "D (disk sleep)"
 *   top -p <PID>,<PID>,<PID>          the S column is the state. q quits.
 *
 *   If a run ever gets cut off and leaves processes behind, pkill 14_state_codes
 *   cleans them up.
 */
/* _GNU_SOURCE asks glibc to turn on its Linux extras in the headers below. */
#define _GNU_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_KIDS 6 /* one child for each state letter in the table above */

/* One entry per child: its PID, and a note on how it got into its state. */
struct kid {
    pid_t pid;
    const char *how;
};

/*
 * Field 3 of /proc/<PID>/stat is the state letter. It follows the ")" that ends the name.
 * The line looks like "1234 (14_state_codes) S 1200 ...". If anything goes wrong we
 * return '?' instead. "static" on a function means it's only used in this file.
 */
static char kernel_state(pid_t pid)
{
    char path[64];
    char line[512];
    FILE *stat_file;
    char *name_end;

    /*
     * snprintf is printf into a char array. sizeof(path) is the array's size, so it can't
     * write past the end. The (long) cast makes the PID match %ld. (void) means we're
     * ignoring the return value on purpose.
     */
    (void)snprintf(path, sizeof(path), "/proc/%ld/stat", (long)pid);
    stat_file = fopen(path, "r"); /* NULL means the file couldn't be opened */
    if (stat_file == NULL) {
        return '?';
    }
    if (fgets(line, sizeof(line), stat_file) == NULL) {
        (void)fclose(stat_file);
        return '?';
    }
    (void)fclose(stat_file);

    /*
     * strrchr finds the LAST ")" in the line and returns a pointer to it. Searching from
     * the end matters in case the program name itself has a ")" in it. From there,
     * name_end[1] is the space after it and name_end[2] is the state letter.
     */
    name_end = strrchr(line, ')');
    return (name_end != NULL && name_end[1] == ' ') ? name_end[2] : '?';
}

/*
 * Kill every child and wait for each one, so nothing gets left behind. SIGKILL can't be
 * caught or ignored. waitpid() then reaps each child so none stays a zombie.
 */
static void kill_and_reap(const struct kid *kids, int count)
{
    for (int i = 0; i < count; i++) {
        (void)kill(kids[i].pid, SIGKILL);
    }
    /*
     * The D child (kids[2]) has a vfork() child of its own. setpgid() put the two of
     * them in their own process group, and a negative PID means "everyone in that group".
     */
    if (count > 2) {
        (void)kill(-kids[2].pid, SIGKILL);
    }
    for (int i = 0; i < count; i++) {
        (void)waitpid(kids[i].pid, NULL, 0);
    }
}

/*
 * fork() returns -1 when it fails. Never keep -1 as a child's PID: kill(-1, SIGKILL)
 * means "every process I'm allowed to signal", and that would log you out. So if fork
 * failed, clean up the children we already made and quit.
 */
static void quit_if_fork_failed(pid_t pid, const struct kid *kids, int count)
{
    if (pid >= 0) {
        return;
    }
    perror("fork"); /* prints "fork: " plus the reason it failed */
    kill_and_reap(kids, count);
    exit(1);
}

/* Does nothing on purpose. It's only here so a signal interrupts getchar() (see main). */
static void stop_waiting(int signo)
{
    (void)signo; /* we don't use signo, and (void) says that's on purpose */
}

int main(void)
{
    struct kid kids[MAX_KIDS];
    int count = 0;
    pid_t pid;
    struct sigaction sa;

    /*
     * R: a busy loop always wants the CPU.
     * (pid = fork()) == 0 does two things: it saves fork's return value in pid, then
     * checks it. 0 means we're the child, so only the child goes into the braces.
     */
    if ((pid = fork()) == 0) {
        /* volatile makes the compiler really update spins in memory every time around. */
        volatile unsigned long spins = 0;
        for (;;) {
            spins++;
        }
    }
    /*
     * The parent records the child. (struct kid){pid, "..."} builds a struct on the spot,
     * and kids[count++] stores it in slot count and then adds 1 to count.
     */
    quit_if_fork_failed(pid, kids, count);
    kids[count++] = (struct kid){pid, "busy loop"};

    /* S: pause() sleeps until any signal shows up. */
    if ((pid = fork()) == 0) {
        for (;;) {
            pause();
        }
    }
    quit_if_fork_failed(pid, kids, count);
    kids[count++] = (struct kid){pid, "pause()"};

    /*
     * D: after vfork(), the parent sleeps until its child calls exec or exits, and
     * Linux counts that sleep as D. A vfork() child is supposed to exec or _exit right
     * away. We break that rule on purpose and have it sleep for an hour, so the parent
     * stays in D. setpgid() gives the pair their own process group, so cleanup can kill
     * both with one signal.
     */
    if ((pid = fork()) == 0) {
        (void)setpgid(0, 0);
        if (vfork() == 0) {
            sleep(3600);
            _exit(0);
        }
        _exit(0);
    }
    quit_if_fork_failed(pid, kids, count);
    kids[count++] = (struct kid){pid, "vfork() parent waiting"};

    /* T: SIGSTOP stops a process. Ctrl-Z does nearly the same thing with SIGTSTP. */
    if ((pid = fork()) == 0) {
        raise(SIGSTOP);
        for (;;) {
            pause();
        }
    }
    quit_if_fork_failed(pid, kids, count);
    kids[count++] = (struct kid){pid, "raise(SIGSTOP)"};

    /*
     * t: PTRACE_TRACEME makes the parent this child's debugger. A traced process that
     * gets a signal stops and waits for its debugger, which shows up as t instead of T.
     */
    if ((pid = fork()) == 0) {
        (void)ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        raise(SIGSTOP);
        for (;;) {
            pause();
        }
    }
    quit_if_fork_failed(pid, kids, count);
    kids[count++] = (struct kid){pid, "ptrace + SIGSTOP"};

    /* Z: exits right away, and we don't call waitpid() on it until cleanup. */
    if ((pid = fork()) == 0) {
        _exit(0);
    }
    quit_if_fork_failed(pid, kids, count);
    kids[count++] = (struct kid){pid, "exited, not reaped"};

    sleep(1); /* give every child a moment to get into its state */

    /* %-7s and %-6c pad each value to a fixed width, so the columns line up. */

    printf("Parent PID %ld created %d children:\n\n", (long)getpid(), count);
    printf("  %-7s %-6s %s\n", "PID", "state", "how the child got there");
    for (int i = 0; i < count; i++) {
        printf("  %-7ld %-6c %s\n", (long)kids[i].pid, kernel_state(kids[i].pid), kids[i].how);
    }

    printf("\nIn another terminal, ps shows the same letters:\n  ps -o pid,ppid,stat,wchan:24,cmd -p ");
    for (int i = 0; i < count; i++) {
        printf("%ld%s", (long)kids[i].pid, i + 1 < count ? "," : "\n");
    }
    printf("  ps -eo pid,stat,comm | awk '$2 ~ /^I/' | head -3     # I: idle kernel threads\n");

    /*
     * Catch Ctrl-C (SIGINT) while we wait, and also a plain kill (SIGTERM) and the
     * terminal window closing (SIGHUP). Without this, any of those would kill this
     * parent on the spot, and the D child would be left running for an hour, because
     * it sits in its own process group where those signals don't reach. The handler
     * does nothing. Since sa_flags doesn't include SA_RESTART, the signal just makes
     * getchar() give up, and we carry on to the cleanup as if you'd pressed Enter.
     */
    /*
     * memset zeroes the struct first. sa_handler is the function to run, sa_mask lists
     * signals to hold off while it runs (none), and sa_flags = 0 means no SA_RESTART.
     */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = stop_waiting;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    (void)sigaction(SIGINT, &sa, NULL);
    (void)sigaction(SIGTERM, &sa, NULL);
    (void)sigaction(SIGHUP, &sa, NULL);

    /* fflush pushes the prompt out now, before getchar() blocks waiting for you. */
    printf("\nPress Enter (or Ctrl-C) to kill and reap every child.\n");
    fflush(stdout);
    (void)getchar();

    kill_and_reap(kids, count);
    printf("All children reaped.\n");
    return 0;
}
