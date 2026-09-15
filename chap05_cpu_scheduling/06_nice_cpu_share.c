/*
 * Chapter 5: CPU Scheduling
 * Demo 06: Niceness decides CPU share
 *
 * What this demo shows:
 * - Two CPU-bound children forced onto the same CPU compete for it.
 * - The child with the higher nice value (lower priority) receives much less CPU
 *   time (Fox 4.5). Linux's scheduler gives each nice level a weight: nice 0 is 1024,
 *   nice 10 is 110, so the split is about 90% : 10%.
 * - An ordinary user may raise niceness but may not lower it (Permission denied).
 *
 * Run:
 *   ./06_nice_cpu_share             nice 0 vs nice 10 for 3 seconds
 *   ./06_nice_cpu_share 19 5        nice 0 vs nice 19 for 5 seconds
 * While it runs, in another terminal:
 *   ps -o pid,ni,pri,psr,%cpu,cmd -C 06_nice_cpu_sh      (names are cut to 15 characters)
 */
#define _GNU_SOURCE
#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* Scheduler weight for nice 0..19 (kernel/sched/core.c, sched_prio_to_weight). */
static const double nice_weight[20] = {
    1024, 820, 655, 526, 423, 335, 272, 215, 172, 137,
    110, 87, 70, 56, 45, 36, 29, 23, 18, 15,
};

static double seconds(struct timeval tv)
{
    return (double)tv.tv_sec + (double)tv.tv_usec / 1e6;
}

static pid_t start_worker(int niceness, int run_seconds)
{
    pid_t pid = fork();

    if (pid != 0) {
        return pid;
    }

    cpu_set_t first_cpu;
    CPU_ZERO(&first_cpu);
    CPU_SET(0, &first_cpu);
    if (sched_setaffinity(0, sizeof(first_cpu), &first_cpu) < 0) {
        perror("sched_setaffinity");
        _exit(1);
    }
    if (setpriority(PRIO_PROCESS, 0, niceness) < 0) {
        perror("setpriority");
        _exit(1);
    }

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
    int low_nice = 10;
    int run_seconds = 3;

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

    double normal_cpu = seconds(normal_usage.ru_utime) + seconds(normal_usage.ru_stime);
    double polite_cpu = seconds(polite_usage.ru_utime) + seconds(polite_usage.ru_stime);
    double total = normal_cpu + polite_cpu;
    double predicted = 100.0 * nice_weight[low_nice] / (nice_weight[0] + nice_weight[low_nice]);

    printf("  nice  0  PID %-6ld CPU time %5.2f s  %5.1f%%\n", (long)normal, normal_cpu, 100.0 * normal_cpu / total);
    printf("  nice %2d  PID %-6ld CPU time %5.2f s  %5.1f%%\n", low_nice, (long)polite, polite_cpu, 100.0 * polite_cpu / total);
    printf("  scheduler weights predict %.1f%% for nice %d\n", predicted, low_nice);

    if (setpriority(PRIO_PROCESS, 0, -5) < 0) {
        printf("\nsetpriority(this process, nice -5) as UID %ld: %s\n", (long)getuid(), strerror(errno));
        printf("  lowering niceness (raising priority) requires root\n");
    } else {
        printf("\nsetpriority(this process, nice -5) succeeded: running as root\n");
    }
    return 0;
}
