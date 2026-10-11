/*
 * Chapter 5: CPU Scheduling
 * Demo 01: A dispatcher you can watch
 *
 * What's going on
 *   The parent process plays the short-term scheduler. It forks four CPU-bound
 *   children and decides which one may run: SIGCONT dispatches a child, and SIGSTOP
 *   preempts it. Every child is pinned to CPU 0, so they share one CPU, and only one
 *   is ever un-stopped at a time.
 *
 *   The workload is the SRTF example from the Chapter 5 slides:
 *     P1 arrives at 0, burst 8    P2 arrives at 1, burst 4
 *     P3 arrives at 2, burst 9    P4 arrives at 3, burst 5
 *   One time unit is 100 ms of real time (1 s with -s). A child exits only after it
 *   has used its whole burst of CPU time, and the parent records when each one
 *   actually exited. So the table at the end is measured, not calculated.
 *
 *   The Linux kernel has no SJF or SRTF policy, because it can't know how long a
 *   burst will be. This dispatcher is told the bursts in advance, which is the same
 *   assumption SJF makes on paper.
 *
 * Build
 *   make
 *
 * Run (no sudo needed: a process may stop and continue its own children)
 *   ./01_dispatcher fcfs
 *   ./01_dispatcher sjf
 *   ./01_dispatcher srtf
 *   ./01_dispatcher rr 4         round robin, quantum 4 units
 *   ./01_dispatcher -s srtf      slow: 1 unit = 1 s, so a run takes 26 s
 *
 * What to look for
 *   The SIGSTOP and SIGCONT lines are the Gantt chart, printed as it happens. They
 *   match the charts on the "Same Processes, Four Algorithms" slide, and the
 *   averages match the scorecard slide after it:
 *                 turnaround   waiting   response
 *     fcfs           15.25       8.75      8.75
 *     sjf            14.25       7.75      7.75
 *     srtf           13.00       6.50      4.25
 *     rr 4           18.25      11.75      4.50
 *   Measured runs come out a few hundredths higher, the time the signals and the
 *   parent's own work take.
 *
 * Other ways to see it
 *   During a slow run (-s), in another terminal:
 *     watch -n 0.5 ps -o pid,stat,psr,time,cmd -C 01_dispatcher
 *   STAT T means stopped, R means running. One child at a time is R, and its TIME
 *   column is the only one going up. PSR is the CPU it is on: 0 for every child.
 *   The first row is the parent, asleep (S) between time units.
 *   A child disappears from the list once it exits.
 */
#define _GNU_SOURCE    /* needed for sched_setaffinity() and the cpu_set_t macros */
#include <errno.h>     /* EINTR */
#include <sched.h>     /* cpu_set_t, sched_setaffinity() */
#include <signal.h>    /* kill(), raise(), SIGSTOP, SIGCONT, SIGKILL */
#include <stdio.h>     /* printf(), fprintf(), perror() */
#include <stdlib.h>    /* strtol() */
#include <string.h>    /* strcmp() */
#include <sys/prctl.h> /* prctl(), PR_SET_PDEATHSIG */
#include <sys/wait.h>  /* waitpid(), WUNTRACED */
#include <time.h>      /* clock_gettime(), clock_nanosleep() */
#include <unistd.h>    /* fork(), _exit() */

static int unit_ms = 100;   /* one time unit, in milliseconds of real time; -s makes it 1000 */

/* What the dispatcher knows about one process, like one row of a PCB table. */
struct proc {
    const char *name;
    int arrival;          /* time units */
    int burst;            /* time units of CPU */
    int remaining;        /* units the dispatcher has not yet given it */
    pid_t pid;            /* the child's process ID, used to send it signals */
    int finished;         /* 1 once the child has exited */
    double first_run;     /* measured: when it was first dispatched, or -1 before that */
    double completion;    /* measured, in time units */
};

/* Same workload as the Chapter 5 slides (the SRTF example). */
static struct proc p[] = {
    {"P1", 0, 8, 8, 0, 0, -1.0, 0.0},
    {"P2", 1, 4, 4, 0, 0, -1.0, 0.0},
    {"P3", 2, 9, 9, 0, 0, -1.0, 0.0},
    {"P4", 3, 5, 5, 0, 0, -1.0, 0.0},
};
static const int n = (int)(sizeof(p) / sizeof(p[0])); /* number of processes: 4 */

static struct timespec start; /* the moment the schedule starts: time 0 */

/*
 * Round robin ready queue: a circular array. rr_push() adds a process at the tail and
 * rr_pop() takes one from the head. % 16 wraps the index back to 0 at the end.
 */
static int rr_queue[16];
static int rr_head, rr_count;

static void rr_push(int i) { rr_queue[(rr_head + rr_count++) % 16] = i; }
static int rr_pop(void) { int i = rr_queue[rr_head]; rr_head = (rr_head + 1) % 16; rr_count--; return i; }

/* How many time units have passed since start, as a decimal number. */
static double now_units(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return ((double)(t.tv_sec - start.tv_sec) * 1000.0 +
            (double)(t.tv_nsec - start.tv_nsec) / 1e6) / unit_ms;
}

/*
 * Sleep until time unit `unit` begins. TIMER_ABSTIME means "until this clock time",
 * not "for this long", so small delays in one step don't pile up over the run.
 */
static void sleep_until(int unit)
{
    struct timespec t = start;
    long long ns = (long long)t.tv_nsec + (long long)unit * unit_ms * 1000000LL;
    t.tv_sec += (time_t)(ns / 1000000000LL);
    t.tv_nsec = (long)(ns % 1000000000LL);
    while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL) == EINTR) {
    }
}

/* Child: wait, stopped, until dispatched; then use `burst` units of CPU time and exit. */
static void worker(int burst)
{
    cpu_set_t cpu0;
    CPU_ZERO(&cpu0);
    CPU_SET(0, &cpu0);
    if (sched_setaffinity(0, sizeof(cpu0), &cpu0) < 0) {
        perror("sched_setaffinity");
        _exit(1);
    }
    prctl(PR_SET_PDEATHSIG, SIGKILL);     /* if the parent dies, kill this child too */
    raise(SIGSTOP);                       /* stop ourselves until the parent sends SIGCONT */

    /*
     * Spin until this process has used `burst` units of CPU time. CLOCK_PROCESS_CPUTIME_ID
     * only counts time actually spent running, so time spent stopped doesn't count.
     * volatile stops the compiler from deleting the loop as useless.
     */
    struct timespec cpu;
    volatile unsigned long spins = 0;
    do {
        for (int i = 0; i < 100000; i++) {        /* user-mode work between clock checks */
            spins++;
        }
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cpu);
    } while ((double)cpu.tv_sec * 1000.0 + (double)cpu.tv_nsec / 1e6 < burst * unit_ms);
    _exit(0);
}

/* A process is ready if it has arrived and hasn't finished. */
static int arrived(int i, int unit)
{
    return !p[i].finished && p[i].arrival <= unit;
}

/*
 * The scheduling policies. Each one returns the index of the process to run for the
 * next time unit, or -1 for none. `running` is the process that ran last unit, or -1.
 */

/* FCFS: keep the current process; otherwise pick the earliest arrival. */
static int pick_fcfs(int running, int unit)
{
    if (running != -1) return running;               /* non-preemptive */
    int best = -1;
    for (int i = 0; i < n; i++)
        if (arrived(i, unit) && (best == -1 || p[i].arrival < p[best].arrival)) best = i;
    return best;
}

/* SJF: keep the current process; otherwise pick the shortest burst. */
static int pick_sjf(int running, int unit)
{
    if (running != -1) return running;               /* non-preemptive */
    int best = -1;
    for (int i = 0; i < n; i++)
        if (arrived(i, unit) && (best == -1 || p[i].burst < p[best].burst)) best = i;
    return best;
}

/* SRTF: every unit, pick the least remaining time, even if that preempts `running`. */
static int pick_srtf(int running, int unit)
{
    int best = running;                              /* ties keep the CPU */
    for (int i = 0; i < n; i++)
        if (arrived(i, unit) && (best == -1 || p[i].remaining < p[best].remaining)) best = i;
    return best;
}

/* RR: keep the current process until it has used a full quantum, then rotate. */
static int pick_rr(int running, int used, int quantum)
{
    if (running != -1 && used < quantum) return running;
    if (running != -1) rr_push(running);             /* quantum expired: to the tail */
    return rr_count > 0 ? rr_pop() : -1;
}

int main(int argc, char *argv[])
{
    const char *prog = argv[0];

    /* Read the command line: an optional -s, then the policy, then the RR quantum. */
    if (argc > 1 && strcmp(argv[1], "-s") == 0) {
        unit_ms = 1000;
        argv++;
        argc--;
    }
    const char *policy = argc > 1 ? argv[1] : "fcfs";
    int quantum = argc > 2 ? (int)strtol(argv[2], NULL, 10) : 4;

    if (strcmp(policy, "fcfs") && strcmp(policy, "sjf") && strcmp(policy, "srtf") &&
        strcmp(policy, "rr")) {
        fprintf(stderr, "Usage: %s [-s] fcfs|sjf|srtf|rr [quantum]\n", prog);
        return 2;
    }
    if (quantum < 1) quantum = 1;

    /*
     * Create every process up front, stopped. A process doesn't count as arrived until
     * its arrival time, so the dispatcher won't pick it before then.
     */
    for (int i = 0; i < n; i++) {
        p[i].pid = fork();
        if (p[i].pid < 0) {
            perror("fork");
            return 1;
        }
        if (p[i].pid == 0) worker(p[i].burst);
        waitpid(p[i].pid, NULL, WUNTRACED);          /* wait until it has stopped */
    }

    if (strcmp(policy, "rr") == 0) printf("Policy rr, quantum %d (1 unit = %d ms)\n", quantum, unit_ms);
    else printf("Policy %s (1 unit = %d ms)\n", policy, unit_ms);
    printf("   time  event\n");

    clock_gettime(CLOCK_MONOTONIC, &start);
    int running = -1, used = 0, done = 0; /* who has the CPU, units used of its quantum, exits so far */

    /* One trip through this loop is one time unit. */
    for (int unit = 0; done < n; unit++) {
        if (strcmp(policy, "rr") == 0)               /* new arrivals join the queue before a preempted process */
            for (int i = 0; i < n; i++)
                if (p[i].arrival == unit) rr_push(i);

        int next;
        if (strcmp(policy, "fcfs") == 0) next = pick_fcfs(running, unit);
        else if (strcmp(policy, "sjf") == 0) next = pick_sjf(running, unit);
        else if (strcmp(policy, "srtf") == 0) next = pick_srtf(running, unit);
        else next = pick_rr(running, used, quantum);

        if (next != running || used == quantum)      /* new process, or a fresh RR quantum */
            used = 0;
        /* A context switch: stop the old process and continue the new one. */
        if (next != running) {
            if (running != -1) {
                kill(p[running].pid, SIGSTOP);
                printf("  %5.2f  SIGSTOP %s (preempted, %d left)\n", now_units(), p[running].name,
                       p[running].remaining);
            }
            if (next != -1) {
                kill(p[next].pid, SIGCONT);
                if (p[next].first_run < 0) {
                    p[next].first_run = now_units(); /* its first time on the CPU */
                }
                printf("  %5.2f  SIGCONT %s (pid %ld)\n", now_units(), p[next].name, (long)p[next].pid);
            }
            running = next;
        }

        sleep_until(unit + 1);                       /* let it run for one unit */

        /* The process that just ran used one more unit. */
        if (running != -1) {
            p[running].remaining--;
            used++;
            if (p[running].remaining == 0) {
                waitpid(p[running].pid, NULL, 0);    /* it exits once its CPU time is used */
                p[running].completion = now_units();
                p[running].finished = 1;
                printf("  %5.2f  %s exited\n", p[running].completion, p[running].name);
                done++;
                running = -1;
            }
        }
    }

    /*
     * The same measures as the slides' scorecard:
     *   turnaround = completion - arrival
     *   waiting    = turnaround - burst
     *   response   = first run - arrival
     */
    double total_turnaround = 0.0, total_wait = 0.0, total_response = 0.0;
    printf("\n%-4s %-7s %-5s %-10s %-10s %-7s %-8s\n", "Proc", "Arrival", "Burst", "Completion",
           "Turnaround", "Wait", "Response");
    for (int i = 0; i < n; i++) {
        double turnaround = p[i].completion - p[i].arrival;
        double wait = turnaround - p[i].burst;
        double response = p[i].first_run - p[i].arrival;
        printf("%-4s %-7d %-5d %-10.2f %-10.2f %-7.2f %-8.2f\n", p[i].name, p[i].arrival, p[i].burst,
               p[i].completion, turnaround, wait, response);
        total_turnaround += turnaround;
        total_wait += wait;
        total_response += response;
    }
    printf("\nAverage turnaround time = %.2f units (measured)\n", total_turnaround / n);
    printf("Average waiting time    = %.2f units (measured)\n", total_wait / n);
    printf("Average response time   = %.2f units (measured)\n", total_response / n);
    return 0;
}
