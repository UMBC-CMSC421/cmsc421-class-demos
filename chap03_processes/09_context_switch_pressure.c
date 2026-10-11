/*
 * Chapter 3: Processes
 * Demo 09: What it costs to make lots of processes
 *
 * What's going on
 *   This makes a pile of child processes as fast as it can. Each child exits the
 *   moment it starts, and then the parent reaps all of them. Every create, exit, and
 *   reap is work for the kernel: copying the process, scheduling it, cleaning up after
 *   it. This isn't a precise benchmark. It's here to give you a feel for what a
 *   process costs.
 *
 * Build
 *   make
 *
 * Run
 *   ./09_context_switch_pressure          100 children (the default)
 *   ./09_context_switch_pressure 500      any number from 1 to 500
 *
 * What to look for
 *   The program times itself and asks the kernel (getrusage) for its own counters:
 *     - how long each fork + exit + reap took, on average. Compare 50 with 500: the
 *       per-child cost stays about the same, so the total grows with the count.
 *     - the parent's CPU time: almost all of it is sys, time the kernel spent working
 *       for us (copying the process, tearing it down), and user is close to 0. This
 *       program barely runs any code of its own.
 *     - the parent's context switches. A voluntary one happens when wait() has to block
 *       because no child has exited yet. On a multi-core machine the children run and
 *       exit on other CPUs while the parent is still forking, so the count is often 0.
 *       Try  taskset -c 0 ./09_context_switch_pressure 50  to put everything on one
 *       CPU. Now the children have to take turns with the parent, and the numbers
 *       change. The exact counts depend on the scheduler, which is the point.
 *
 * Other ways to see it
 *   time ./09_context_switch_pressure 500
 *       real is wall-clock time. user is time spent running program code. sys is time
 *       the kernel spent working for it (fork, exit, wait). Compare 50 with 500.
 *   /usr/bin/time -v ./09_context_switch_pressure 500
 *       the long version of time. Find "Voluntary context switches", then run it again
 *       with 50. The count goes up with the number of children.
 *   strace -c ./09_context_switch_pressure 50
 *       a table of the system calls the parent made. wait4 (wait's real name on Linux)
 *       shows 50 calls, one per child. clone (fork's real name) shows more than 50,
 *       with some errors. Those are forks that got interrupted and retried on their
 *       own. It's a side effect of strace watching the program, not a bug.
 *   vmstat 1
 *       run it in a second terminal. In the first, run the demo in a loop:
 *         for i in $(seq 20); do ./09_context_switch_pressure 500; done
 *       and watch the cs column (context switches per second) jump. Ctrl-C stops vmstat.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/resource.h> /* getrusage() */
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX_CHILDREN 500L /* a cap, so nobody floods the machine with processes by accident */

/*
 * Turn the command-line text into a number, and refuse anything that isn't 1 to 500.
 * "static" on a function just means it's only used inside this file.
 */
static long parse_count(const char *s)
{
    char *end = NULL;
    errno = 0; /* library calls leave error codes in errno, so clear it before strtol */
    long value = strtol(s, &end, 10);

    /*
     * strtol stops at the first character that isn't a digit and points end at it:
     *   end == s        there were no digits at all, like "abc"
     *   *end != '\0'    there was junk after the number, like "12abc"
     *   ERANGE          the number was too big to fit in a long
     */
    if (errno == ERANGE || end == s || *end != '\0' || value <= 0 ||
        value > MAX_CHILDREN) {
        fprintf(stderr, "Child count must be an integer from 1 through %ld: %s\n",
                MAX_CHILDREN, s);
        exit(1);
    }

    return value;
}

/*
 * A struct timeval (seconds + microseconds) as a double number of seconds.
 * The (double) casts turn the whole numbers into decimals before we divide.
 */
static double seconds_of(struct timeval tv)
{
    return (double)tv.tv_sec + (double)tv.tv_usec / 1e6;
}

int main(int argc, char *argv[])
{
    long count = 100; /* used when you don't give a number */

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [child-count]\n", argv[0]);
        return 2;
    }

    if (argc == 2) {
        count = parse_count(argv[1]);
    }

    long created = 0; /* how many children we actually managed to make */
    int fork_failed = 0;
    struct timespec start;
    struct timespec end;

    /*
     * CLOCK_MONOTONIC is a stopwatch: it only moves forward, even if someone changes
     * the date.
     * The (void) in front says we're ignoring the return value on purpose.
     */
    (void)clock_gettime(CLOCK_MONOTONIC, &start);

    /*
     * Make the children. fork() makes a copy of this process. From that line on, two
     * processes are running this code. In the child, fork() returns 0. In the parent, it
     * returns the child's PID (pid_t is just the type that holds a process ID).
     */
    for (long i = 0; i < count; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            /*
             * Out of processes or memory. Stop making more, but still reap the ones we
             * have.
             * perror prints our label plus the reason the last call failed.
             */
            perror("fork");
            fork_failed = 1;
            break;
        }

        /*
         * Child: exit the moment we start. _exit() ends the process right away. Plain
         * exit() would also flush the stdio buffers the child copied from the parent.
         */
        if (pid == 0) {
            _exit(0);
        }

        created++; /* parent: count it and go around again */
    }

    /*
     * Reap them. wait(NULL) collects any one finished child, so call it once per child.
     * NULL means we don't care how each child exited. wait() returns -1 on an error.
     */
    for (long i = 0; i < created; i++) {
        if (wait(NULL) < 0) {
            perror("wait");
            return 1;
        }
    }

    /* Stop the stopwatch. Elapsed is end minus start: whole seconds plus nanoseconds. */
    (void)clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / 1e9;

    /*
     * getrusage() returns the kernel's bookkeeping for a process. RUSAGE_SELF is this
     * process. RUSAGE_CHILDREN is all the children we've reaped, added together.
     * user time is our own code running. sys time is the kernel doing work for us.
     */
    struct rusage self;
    struct rusage children;
    (void)getrusage(RUSAGE_SELF, &self);
    (void)getrusage(RUSAGE_CHILDREN, &children);

    printf("Created and reaped %ld short-lived children in %.3f s", created, elapsed);
    if (created > 0) {
        printf(" (about %.0f microseconds each)", elapsed * 1e6 / (double)created);
    }
    printf("\n");
#ifdef __linux__ /* these two counters are a Linux extra, not standard POSIX */
    printf("  parent context switches:  %ld voluntary, %ld involuntary\n", self.ru_nvcsw, self.ru_nivcsw);
#endif
    printf("  parent CPU time:          user %.3f s, sys %.3f s\n",
           seconds_of(self.ru_utime), seconds_of(self.ru_stime));
    printf("  children's CPU time:      user %.3f s, sys %.3f s\n",
           seconds_of(children.ru_utime), seconds_of(children.ru_stime));
    printf("A rough feel for what a process costs, not a precise benchmark.\n");
    /* cond ? a : b gives a when cond is true, b when it's false. So 1 if a fork failed. */
    return fork_failed ? 1 : 0;
}
