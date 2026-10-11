/*
 * Chapter 5: CPU Scheduling
 * Demo 03: The nice value decides each process's share of the CPU
 *
 * What's going on
 *   Two CPU-bound children are pinned to the same CPU, so they have to share it.
 *   One runs at nice 0, the other at a higher nice value, which means lower
 *   priority (Fox 4.5). The Linux scheduler (CFS on the course VM) gives each nice
 *   level a weight, and each process gets CPU time in proportion to its weight.
 *   CPU share = my weight / total weight. The "Nice Values Become Weights" slide
 *   uses nice 0 (weight 1024) and nice 5 (weight 335): 1024 / 1359 = 75% and
 *   335 / 1359 = 25%. That's the default here.
 *
 *   At the end the parent tries to lower its own nice value to -5. An ordinary user
 *   may raise a nice value but not lower it, so that fails unless you are root.
 *
 * Build
 *   make
 *
 * Run
 *   ./03_nice_cpu_share             nice 0 vs nice 5 for 3 seconds
 *   ./03_nice_cpu_share 10          nice 0 vs nice 10
 *   ./03_nice_cpu_share 19 5        nice 0 vs nice 19 for 5 seconds
 *
 * What to look for
 *   The measured percentages land close to the one the weights predict. On the course
 *   VM nice 10 got 9.9% against a prediction of 9.7%.
 *   The last line says Permission denied when you are not root.
 *
 * Other ways to see it
 *   While it runs, in another terminal:
 *     ps -o pid,ni,pri,psr,%cpu,cmd -C 03_nice_cpu_sha
 *   (ps matches only the first 15 characters of a program name.) NI is the nice
 *   value and PSR the CPU: 0 for both children.
 */
#define _GNU_SOURCE       /* needed for sched_setaffinity(), wait4(), and the cpu_set_t macros */
#include <errno.h>        /* errno */
#include <sched.h>        /* cpu_set_t, sched_setaffinity() */
#include <stdio.h>        /* printf(), fprintf(), perror() */
#include <stdlib.h>       /* strtol() */
#include <string.h>       /* strerror() */
#include <sys/resource.h> /* setpriority(), struct rusage */
#include <sys/time.h>     /* struct timeval */
#include <sys/wait.h>     /* wait4() */
#include <time.h>         /* clock_gettime() */
#include <unistd.h>       /* fork(), getuid(), _exit() */

/* The scheduler's weight for nice 0 to 19, copied from sched_prio_to_weight in the kernel source. */
static const double nice_weight[20] = {
    1024, 820, 655, 526, 423, 335, 272, 215, 172, 137,
    110, 87, 70, 56, 45, 36, 29, 23, 18, 15,
};

/* Convert a timeval (seconds plus microseconds) to seconds. */
static double seconds(struct timeval tv)
{
    return (double)tv.tv_sec + (double)tv.tv_usec / 1e6;
}

/* Fork a child that pins itself to CPU 0, sets its nice value, and spins for run_seconds. */
static pid_t start_worker(int niceness, int run_seconds)
{
    pid_t pid = fork();

    if (pid != 0) {
        return pid; /* the parent (or a failed fork, -1) goes back to main */
    }

    /* Child: allow only CPU 0. */
    cpu_set_t first_cpu;
    CPU_ZERO(&first_cpu);
    CPU_SET(0, &first_cpu);
    if (sched_setaffinity(0, sizeof(first_cpu), &first_cpu) < 0) {
        perror("sched_setaffinity");
        _exit(1);
    }
    /* Set this process's nice value. 0 means "this process". */
    if (setpriority(PRIO_PROCESS, 0, niceness) < 0) {
        perror("setpriority");
        _exit(1);
    }

    /*
     * Spin until run_seconds of real time have passed. Both children stop at the same
     * moment, so the CPU time each one got shows its share. volatile keeps the compiler
     * from deleting the loop as useless.
     */
    struct timespec now;
    struct timespec deadline;
    volatile unsigned long spins = 0;

    (void)clock_gettime(CLOCK_MONOTONIC, &deadline);
    deadline.tv_sec += run_seconds;
    do {
        for (int i = 0; i < 1000000; i++) {
            spins++;
        }
        (void)clock_gettime(CLOCK_MONOTONIC, &now);
    } while (now.tv_sec < deadline.tv_sec ||
             (now.tv_sec == deadline.tv_sec && now.tv_nsec < deadline.tv_nsec));
    _exit(0);
}

int main(int argc, char *argv[])
{
    int low_nice = 5;
    int run_seconds = 3;

    /* Optional arguments: the second child's nice value, then how many seconds to run. */
    if (argc > 1) {
        low_nice = (int)strtol(argv[1], NULL, 10);
    }
    if (argc > 2) {
        run_seconds = (int)strtol(argv[2], NULL, 10);
    }
    if (low_nice < 0 || low_nice > 19 || run_seconds < 1 || run_seconds > 30) {
        fprintf(stderr, "Usage: %s [nice 0-19] [seconds 1-30]\n", argv[0]);
        return 2;
    }

    printf("Two busy children pinned to CPU 0 for %d s: nice 0 versus nice %d\n", run_seconds, low_nice);
    fflush(stdout);

    /* Start both children, then wait for each. wait4() also returns the child's CPU time. */
    pid_t normal = start_worker(0, run_seconds);
    pid_t polite = start_worker(low_nice, run_seconds);
    struct rusage normal_usage;
    struct rusage polite_usage;
    int status;

    if (normal < 0 || polite < 0) {
        perror("fork");
        return 1;
    }
    if (wait4(normal, &status, 0, &normal_usage) < 0 || wait4(polite, &status, 0, &polite_usage) < 0) {
        perror("wait4");
        return 1;
    }

    /* CPU time = user time + system time. Compare each child's share with the weights. */
    double normal_cpu = seconds(normal_usage.ru_utime) + seconds(normal_usage.ru_stime);
    double polite_cpu = seconds(polite_usage.ru_utime) + seconds(polite_usage.ru_stime);
    double total = normal_cpu + polite_cpu;
    double predicted = 100.0 * nice_weight[low_nice] / (nice_weight[0] + nice_weight[low_nice]);

    printf("  nice  0  PID %-6ld CPU time %5.2f s  %5.1f%%\n", (long)normal, normal_cpu, 100.0 * normal_cpu / total);
    printf("  nice %2d  PID %-6ld CPU time %5.2f s  %5.1f%%\n", low_nice, (long)polite, polite_cpu, 100.0 * polite_cpu / total);
    printf("  scheduler weights predict %.1f%% for nice %d\n", predicted, low_nice);

    /* Try to lower our own nice value, which only root may do. */
    if (setpriority(PRIO_PROCESS, 0, -5) < 0) {
        printf("\nsetpriority(this process, nice -5) as UID %ld: %s\n", (long)getuid(), strerror(errno));
        printf("  lowering niceness (raising priority) requires root\n");
    } else {
        printf("\nsetpriority(this process, nice -5) succeeded: running as root\n");
    }
    return 0;
}
