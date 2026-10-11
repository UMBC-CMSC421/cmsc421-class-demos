/*
 * Chapter 3: Processes
 * Demo 19: What happens to orphans
 *
 * What's going on
 *   An orphan is a process whose parent exited first. The kernel doesn't leave it
 *   without a parent. It hands the orphan to a new one (Fox 4.6.1):
 *     - the closest ancestor that signed up as a "child subreaper", if there is one
 *     - otherwise PID 1, which on Debian is systemd (it replaced the old init)
 *   Either way, the new parent calls wait(). So when the orphan eventually exits it
 *   gets reaped, instead of staying a zombie forever.
 *
 *   The program builds a chain: this program -> middle -> grandchild. The middle
 *   process exits almost right away, which orphans the grandchild. The grandchild
 *   prints its parent three times so you can watch it change.
 *   Round 1 runs as is. In round 2 this program signs up as a subreaper first.
 *
 * Build
 *   make
 *
 * Run
 *   ./19_orphan_subreaper
 *
 * What to look for
 *   Round 1: the grandchild's first line names the middle process as its parent. Once
 *   middle exits, the parent turns into PID 1 (systemd).
 *   Round 2: once middle exits, the parent is THIS program, and this program is the
 *   one that reaps the orphan (exit status 7).
 *
 *   Seeing a PPID in round 1 that isn't 1? Then something above you is already a
 *   subreaper. Common ones are systemd --user, container init programs like tini,
 *   and some terminal apps. ps -o pid,cmd -p <that PPID> shows who it is.
 *
 * Other ways to see it
 *   bash -c 'sleep 30 & exit'       bash starts a sleep and quits right away,
 *                                   orphaning the sleep
 *   ps -o pid,ppid,cmd -C sleep     that sleep's PPID is now 1 (or a subreaper)
 *   pstree -p | head                systemd(1) is at the top of the whole tree
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* Sleep for the given number of milliseconds. static: a helper only this file uses. */
static void nap_ms(long milliseconds)
{
    /*
     * Split the time into whole seconds plus leftover nanoseconds (1 ms = 1,000,000 ns).
     * (void) on the call below means we're ignoring its return value on purpose.
     */
    struct timespec delay = {.tv_sec = milliseconds / 1000, .tv_nsec = (milliseconds % 1000) * 1000000L};
    (void)nanosleep(&delay, NULL);
}

/* Print our parent's PID and its name. /proc/<PID>/comm holds a process's name. */
static void report_parent(void)
{
    pid_t parent = getppid(); /* asks the kernel: who is my parent right now? */
    char path[64];
    char name[64] = "?";
    FILE *comm;

    /*
     * snprintf() is printf into a buffer. sizeof(path) is the buffer's size, so it
     * can't write past the end. (long) converts pid_t to match %ld.
     */
    (void)snprintf(path, sizeof(path), "/proc/%ld/comm", (long)parent);
    comm = fopen(path, "r"); /* NULL if the file couldn't be opened */
    if (comm != NULL) {
        if (fgets(name, sizeof(name), comm) != NULL) {
            name[strcspn(name, "\n")] = '\0'; /* chop off the newline */
        }
        (void)fclose(comm);
    }
    printf("    grandchild %ld: my parent is PID %ld (%s)\n", (long)getpid(), (long)parent, name);
    fflush(stdout); /* write it out now, so lines from each process stay in order */
}

/* This process -> middle -> grandchild. The middle process exits almost at once. */
static void make_orphan(void)
{
    /* fork() returns 0 in the new child and the child's PID in the parent. */
    pid_t middle = fork();

    if (middle == 0) {
        /* We're the middle process. Make the grandchild. */
        pid_t grandchild = fork();

        if (grandchild == 0) {
            /* Grandchild: check who our parent is, 300 ms apart, then exit with 7. */
            for (int i = 0; i < 3; i++) {
                report_parent();
                nap_ms(300);
            }
            /*
             * _exit() ends the process right away. Children use it instead of exit()
             * so they don't flush a copy of the parent's stdio buffers a second time.
             */
            _exit(7);
        }

        /* Middle: wait a moment so the grandchild's first report still sees us, then leave. */
        nap_ms(100);
        printf("    middle %ld exits, leaving grandchild %ld without a parent\n",
               (long)getpid(), (long)grandchild);
        fflush(stdout);
        _exit(0); /* the moment this runs, the grandchild is an orphan */
    }

    /* Reap middle. The grandchild isn't our child, so this doesn't wait for it. */
    (void)waitpid(middle, NULL, 0);
}

int main(void)
{
    int status;
    pid_t reaped;

    printf("This program is PID %ld.\n\n", (long)getpid());

    printf("1) No subreaper registered:\n");
    fflush(stdout);
    make_orphan();
    nap_ms(1000); /* let the round 1 grandchild finish its three reports */

    printf("\n2) After prctl(PR_SET_CHILD_SUBREAPER, 1), this program adopts its orphaned descendants:\n");
    fflush(stdout);

    /*
     * Sign up as a subreaper: orphans anywhere below us get handed to us, not to PID 1.
     * prctl() returns -1 on failure. perror() then prints "prctl: " plus the reason.
     */
    if (prctl(PR_SET_CHILD_SUBREAPER, 1) < 0) {
        perror("prctl");
        return 1;
    }
    make_orphan();

    /* The orphan is our child now, so we can wait for it. -1 means "any child". */
    reaped = waitpid(-1, &status, 0);
    /* WIFEXITED: it exited normally. WEXITSTATUS then gives its exit code (7). */
    if (reaped > 0 && WIFEXITED(status)) {
        printf("    PID %ld (this program) reaped adopted orphan %ld, exit status %d\n",
               (long)getpid(), (long)reaped, WEXITSTATUS(status));
    }
    return 0;
}
