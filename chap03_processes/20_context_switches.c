/*
 * Chapter 3: Processes
 * Demo 20: Voluntary vs involuntary context switches
 *
 * What's going on
 *   A context switch is the kernel taking the CPU away from one process and handing
 *   it to another. There are two ways it happens (Fox 4.2.2):
 *     voluntary      the process gives up the CPU itself, because it has to wait
 *                    for something: sleep(), reading a file, waiting for a child
 *     involuntary    the process still wanted to run, but its turn was over. A timer
 *                    interrupt fires, the scheduler decides someone else should go,
 *                    and the process gets preempted.
 *   This program runs three children for 3 seconds:
 *     - one that sleeps for 1 ms, over and over
 *     - two busy loops that are both forced onto CPU 0, so they have to take turns
 *   Then it reads each child's two counters from /proc/<PID>/status. It also prints
 *   totals for the whole machine from /proc/stat and /proc/interrupts.
 *
 * Build
 *   make
 *
 * Run
 *   ./20_context_switches
 *
 * What to look for
 *   The sleeper: thousands of voluntary switches and about 0 involuntary.
 *   The two busy loops: 0 voluntary and 100 or more involuntary. Both get about the
 *   same number, because they keep swapping with each other.
 *
 * Other ways to see it
 *   grep ctxt /proc/$$/status    your own shell's two counters. Run a few commands and
 *                                check again. The voluntary one goes up, because the
 *                                shell sleeps while it waits for you to type.
 *   vmstat 1                     one line per second. cs is context switches per
 *                                second and in is interrupts per second. Leave it
 *                                running and start the demo in another terminal.
 *                                Ctrl-C stops it.
 *   pidstat -w 1                 the same counters per process, per second: cswch/s
 *                                is voluntary and nvcswch/s is involuntary. It's in
 *                                the sysstat package.
 */
#define _GNU_SOURCE
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define RUN_SECONDS 3

/* Total context switches since boot, for all processes: the "ctxt" line of /proc/stat. */
static unsigned long long system_context_switches(void)
{
    char line[256];
    unsigned long long count = 0;
    FILE *stat_file = fopen("/proc/stat", "r");

    if (stat_file == NULL) {
        return 0;
    }
    while (fgets(line, sizeof(line), stat_file) != NULL) {
        if (sscanf(line, "ctxt %llu", &count) == 1) {
            break;
        }
    }
    (void)fclose(stat_file);
    return count;
}

/*
 * Timer interrupts on all CPUs, added up. The timer's line in /proc/interrupts is
 * "arch_timer" on ARM machines and "LOC" (local timer) on Intel/AMD. Each line has
 * one count per CPU.
 */
static unsigned long long timer_interrupts(void)
{
    char line[4096];
    unsigned long long total = 0;
    FILE *interrupts = fopen("/proc/interrupts", "r");

    if (interrupts == NULL) {
        return 0;
    }
    while (fgets(line, sizeof(line), interrupts) != NULL) {
        if (strstr(line, "arch_timer") == NULL && strncmp(line, "LOC:", 4) != 0 &&
            strstr(line, " LOC:") == NULL) {
            continue;
        }
        /* Skip past the "NN:" label, then add up numbers until we reach a word. */
        char *token = strtok(strchr(line, ':') + 1, " \t\n");
        while (token != NULL && strspn(token, "0123456789") == strlen(token)) {
            total += strtoull(token, NULL, 10);
            token = strtok(NULL, " \t\n");
        }
    }
    (void)fclose(interrupts);
    return total;
}

/* Read one process's two counters out of /proc/<PID>/status. */
static void switch_counts(pid_t pid, long *voluntary, long *involuntary)
{
    char path[64];
    char line[256];
    FILE *status;

    *voluntary = *involuntary = -1;
    (void)snprintf(path, sizeof(path), "/proc/%ld/status", (long)pid);
    status = fopen(path, "r");
    if (status == NULL) {
        return;
    }
    while (fgets(line, sizeof(line), status) != NULL) {
        (void)sscanf(line, "voluntary_ctxt_switches: %ld", voluntary);
        (void)sscanf(line, "nonvoluntary_ctxt_switches: %ld", involuntary);
    }
    (void)fclose(status);
}

/* Start one child. Returns its PID to the parent, or -1 if fork failed. */
static pid_t start_child(int cpu_bound)
{
    pid_t pid = fork();

    if (pid != 0) {
        return pid; /* parent (or fork failed) */
    }
    if (cpu_bound) {
        cpu_set_t first_cpu;
        volatile unsigned long spins = 0;

        /* Pin this child to CPU 0 only. Two of these can't spread out to other CPUs. */
        CPU_ZERO(&first_cpu);
        CPU_SET(0, &first_cpu);
        (void)sched_setaffinity(0, sizeof(first_cpu), &first_cpu);
        for (;;) {
            spins++;
        }
    }
    /* The sleeper: nap 1 ms, wake up, nap again. Every nap is a voluntary switch. */
    for (;;) {
        struct timespec one_ms = {.tv_sec = 0, .tv_nsec = 1000000L};
        (void)nanosleep(&one_ms, NULL);
    }
}

int main(void)
{
    const char *labels[3] = {"sleeps 1 ms at a time", "CPU-bound, pinned to CPU 0", "CPU-bound, pinned to CPU 0"};
    pid_t children[3];
    unsigned long long switches_before = system_context_switches();
    unsigned long long timer_before = timer_interrupts();

    /* Child 0 is the sleeper. Children 1 and 2 are the busy pair. */
    for (int i = 0; i < 3; i++) {
        children[i] = start_child(i > 0);
        if (children[i] < 0) {
            /*
             * fork failed. Stop the children that did start, then quit. Never pass -1
             * on to kill(): kill(-1, SIGKILL) hits every process you own.
             */
            perror("fork");
            for (int j = 0; j < i; j++) {
                (void)kill(children[j], SIGKILL);
                (void)waitpid(children[j], NULL, 0);
            }
            return 1;
        }
    }

    printf("Running three children for %d seconds...\n\n", RUN_SECONDS);
    fflush(stdout);
    sleep(RUN_SECONDS);

    printf("  %-7s %-28s %10s %12s\n", "PID", "child", "voluntary", "involuntary");
    for (int i = 0; i < 3; i++) {
        long voluntary;
        long involuntary;

        switch_counts(children[i], &voluntary, &involuntary);
        printf("  %-7ld %-28s %10ld %12ld\n", (long)children[i], labels[i], voluntary, involuntary);
    }

    /* Done measuring. Kill and reap all three. */
    for (int i = 0; i < 3; i++) {
        (void)kill(children[i], SIGKILL);
        (void)waitpid(children[i], NULL, 0);
    }

    unsigned long long switches = system_context_switches() - switches_before;
    unsigned long long timers = timer_interrupts() - timer_before;

    printf("\nWhole system over %d s (all %ld CPUs):\n", RUN_SECONDS, sysconf(_SC_NPROCESSORS_ONLN));
    printf("  context switches: %llu  (%llu per second)\n", switches, switches / RUN_SECONDS);
    printf("  timer interrupts: %llu  (%llu per second)\n", timers, timers / RUN_SECONDS);
    return 0;
}
