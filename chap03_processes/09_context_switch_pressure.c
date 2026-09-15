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
 *   The program only prints a one-line summary. The interesting numbers come from the
 *   tools below.
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
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_CHILDREN 500L /* a cap, so nobody floods the machine with processes by accident */

/* Turn the command-line text into a number, and refuse anything that isn't 1 to 500. */
static long parse_count(const char *s)
{
    char *end = NULL;
    errno = 0;
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

    /* Make the children. */
    for (long i = 0; i < count; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            /* Out of processes or memory. Stop making more, but still reap the ones we have. */
            perror("fork");
            fork_failed = 1;
            break;
        }

        if (pid == 0) {
            _exit(0); /* child: exit the moment we start */
        }

        created++; /* parent: count it and go around again */
    }

    /* Reap them. wait(NULL) collects any one finished child, so call it once per child. */
    for (long i = 0; i < created; i++) {
        if (wait(NULL) < 0) {
            perror("wait");
            return 1;
        }
    }

    printf("Created and reaped %ld short-lived children.\n", created);
    printf("This demonstrates fork/exit/wait and scheduler pressure; it is not a pure context-switch benchmark.\n");
    return fork_failed ? 1 : 0;
}
