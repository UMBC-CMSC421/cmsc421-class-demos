/*
 * Chapter 5: CPU Scheduling
 * Demo 02: The Linux kernel's own FCFS, round robin, and priority schedulers
 *
 * What's going on
 *   Here the kernel does the scheduling. Linux has three textbook policies built in
 *   as real-time scheduling classes:
 *     fifo  SCHED_FIFO, all the same priority   = FCFS
 *     rr    SCHED_RR, all the same priority     = round robin, quantum set by the kernel
 *     prio  SCHED_FIFO, different priorities    = priority scheduling
 *   fifo and rr run the SRTF workload from Demo 01 and the slides. prio runs the
 *   "Priority: Step by Step" slide: five jobs that all arrive at time 0.
 *
 *   The parent creates every child up front. Each child sets its policy, moves itself
 *   to CPU 0, and then blocks until the parent releases it at its arrival time. From
 *   then on only the kernel decides who runs. Each child uses up its burst of CPU
 *   time, checking the clock over and over. A jump in the clock means the child
 *   wasn't running, so it records the stretch before the jump. Those stretches are
 *   the timeline the program prints at the end.
 *
 *   Linux real-time priorities run from 1 to 99, and a HIGHER number wins. The slide
 *   uses smaller = higher, so prio mode gives each job Linux priority 6 minus its
 *   slide priority: slide priority 1 becomes 5, the highest of the five.
 *
 *   By default the kernel lets real-time tasks use only 950 ms of every second, so
 *   a runaway real-time task can't lock up the machine. That leaves a 50 ms gap
 *   (half a time unit) in the timeline about once a second.
 *
 * Build
 *   make
 *
 * Run (real-time policies need root, and the demo needs at least 2 CPUs)
 *   sudo ./02_kernel_sched fifo
 *   sudo ./02_kernel_sched rr
 *   sudo ./02_kernel_sched prio
 *   sudo ./02_kernel_sched -s fifo     slow: 1 unit = 1 s
 *
 * What to look for
 *   fifo: P1 0-8, P2 8-12, P3 12-21, P4 21-26, the FCFS chart on the "Same Processes,
 *   Four Algorithms" slide.
 *   prio: P2 0-1, P5 1-6, P1 6-16, P3 16-18, P4 18-19, the chart on the "Priority:
 *   Step by Step" slide.
 *   rr: once two or more have arrived, they take turns one unit (100 ms) at a time.
 *   The kernel's quantum is 1 unit, not the slide's q = 4, so these numbers don't
 *   match the scorecard.
 *
 *   The 50 ms gaps add about half a unit to the averages. With the gaps turned off,
 *   the course VM measured these, against the slides:
 *               turnaround   waiting   response
 *     fifo        15.26        8.76      8.76     slides: 15.25, 8.75, 8.75
 *     prio        12.01        8.21      8.21     slides: 12, 8.2, 8.2
 *   To turn the gaps off, and to put them back afterward:
 *     sudo sysctl kernel.sched_rt_runtime_us=-1
 *     sudo sysctl kernel.sched_rt_runtime_us=950000
 *
 * Other ways to see it
 *   cat /proc/sys/kernel/sched_rr_timeslice_ms     the round robin quantum, in ms
 *   chrt -p <PID>                                  a running process's policy and priority
 */
#define _GNU_SOURCE    /* needed for sched_setaffinity() and the cpu_set_t macros */
#include <sched.h>     /* sched_setscheduler(), sched_setaffinity(), SCHED_FIFO, SCHED_RR */
#include <signal.h>    /* SIGKILL */
#include <stdio.h>     /* printf(), fprintf(), perror(), fopen(), fscanf() */
#include <stdlib.h>    /* qsort() */
#include <string.h>    /* strcmp(), memset() */
#include <sys/mman.h>  /* mmap() */
#include <sys/prctl.h> /* prctl(), PR_SET_PDEATHSIG */
#include <sys/wait.h>  /* wait() */
#include <time.h>      /* clock_gettime() */
#include <unistd.h>    /* fork(), pipe(), read(), write(), usleep(), geteuid(), sysconf(), _exit() */

#define MAX_SEGS 4096 /* room for this many stretches of running time */

struct job {
    const char *name;
    int arrival;          /* time units */
    int burst;            /* time units of CPU */
    int priority;         /* the slide's priority: smaller = higher (prio mode only) */
    int rt_priority;      /* the Linux real-time priority: higher wins */
};

/* fifo and rr: the SRTF workload from the slides, all at real-time priority 10. */
static const struct job srtf_jobs[] = {
    {"P1", 0, 8, 0, 10},
    {"P2", 1, 4, 0, 10},
    {"P3", 2, 9, 0, 10},
    {"P4", 3, 5, 0, 10},
};

/*
 * prio: the "Priority: Step by Step" slide. All five arrive at time 0. The slide's
 * priority 1 is the highest, so it becomes Linux priority 5 (6 - slide priority).
 */
static const struct job prio_jobs[] = {
    {"P1", 0, 10, 3, 3},
    {"P2", 0, 1, 1, 5},
    {"P3", 0, 2, 4, 2},
    {"P4", 0, 1, 5, 1},
    {"P5", 0, 5, 2, 4},
};

static const struct job *jobs; /* the table this run uses */
static int n;                  /* how many jobs are in it */

/* One stretch of time during which a child had the CPU. */
struct seg {
    int job;
    long start_us;
    long end_us;
};

/*
 * Memory the parent and all the children share, so the children can hand their
 * timelines back. mmap() with MAP_SHARED makes memory that fork() does not copy.
 */
struct shared {
    struct timespec start; /* the moment the schedule starts: time 0 */
    int nsegs;             /* how many stretches have been recorded */
    long done_us[8];       /* when each job finished, in microseconds */
    struct seg segs[MAX_SEGS];
};

static int unit_ms = 100;      /* one time unit, in milliseconds; -s makes it 1000 */
static struct timespec *start; /* points at the start time in shared memory */

/* Microseconds of real time since start. */
static long now_us(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (long)(t.tv_sec - start->tv_sec) * 1000000L + (t.tv_nsec - start->tv_nsec) / 1000;
}

/* Microseconds of CPU time this process has used. */
static long cpu_us(void)
{
    struct timespec t;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t);
    return (long)t.tv_sec * 1000000L + t.tv_nsec / 1000;
}

/* Convert microseconds to time units. */
static double units(long us)
{
    return (double)us / 1000.0 / unit_ms;
}

/*
 * Record one stretch in the shared list. Several children can do this at once, so
 * __atomic_fetch_add() hands each caller its own slot number in one indivisible step.
 */
static void add_seg(struct shared *sh, int job, long s, long e)
{
    int k = __atomic_fetch_add(&sh->nsegs, 1, __ATOMIC_SEQ_CST);
    if (k < MAX_SEGS) {
        sh->segs[k] = (struct seg){job, s, e};
    }
}

/* Read one number from a /proc file, or 0 if it can't be read. */
static long read_proc_long(const char *path)
{
    long value = 0;
    FILE *f = fopen(path, "r");
    if (f != NULL) {
        if (fscanf(f, "%ld", &value) != 1) {
            value = 0;
        }
        fclose(f);
    }
    return value;
}

/*
 * Child: become real-time, move to CPU 0, and wait for the parent to release it at its
 * arrival time. Then use `burst` units of CPU and log when it ran.
 */
static void worker(struct shared *sh, int job, int policy, int priority, int ready_fd, int go_fd)
{
    struct sched_param sp = {.sched_priority = priority};
    cpu_set_t cpu0;

    prctl(PR_SET_PDEATHSIG, SIGKILL); /* if the parent dies, kill this child too */
    /* Become a real-time task with this policy and priority, then move to CPU 0. */
    if (sched_setscheduler(0, policy, &sp) < 0) {
        perror("sched_setscheduler");
        _exit(1);
    }
    CPU_ZERO(&cpu0);
    CPU_SET(0, &cpu0);
    if (sched_setaffinity(0, sizeof(cpu0), &cpu0) < 0) {
        perror("sched_setaffinity");
        _exit(1);
    }

    /*
     * Tell the parent this child is set up, then block in read() until the parent
     * writes to this child's pipe. A blocked process isn't ready, so the scheduler
     * ignores it until then: this is how the job "arrives".
     */
    char byte = 'r';
    if (write(ready_fd, &byte, 1) != 1 || read(go_fd, &byte, 1) != 1) {
        _exit(1);
    }

    /*
     * Spin until this child has used its burst of CPU time. Between checks, compare
     * the clock with the last check. A jump of more than 0.5 ms means another job ran
     * in between, so the stretch that just ended goes in the log.
     */
    long cpu_start = cpu_us();
    long seg_start = now_us();
    long last = seg_start;
    volatile unsigned long spins = 0;

    while (cpu_us() - cpu_start < (long)jobs[job].burst * unit_ms * 1000) {
        for (int i = 0; i < 1000; i++) {
            spins++;
        }
        long t = now_us();
        if (t - last > 500) {          /* more than 0.5 ms missing: we were not running */
            add_seg(sh, job, seg_start, last);
            seg_start = t;
        }
        last = t;
    }
    add_seg(sh, job, seg_start, last);
    sh->done_us[job] = last;
    _exit(0);
}

/* qsort() comparison: order stretches by start time. */
static int by_start(const void *a, const void *b)
{
    const struct seg *x = a;
    const struct seg *y = b;
    return (x->start_us > y->start_us) - (x->start_us < y->start_us);
}

int main(int argc, char *argv[])
{
    const char *prog = argv[0];

    /* Read the command line: an optional -s, then the mode. */
    if (argc > 1 && strcmp(argv[1], "-s") == 0) {
        unit_ms = 1000;
        argv++;
        argc--;
    }
    const char *mode = argc > 1 ? argv[1] : "fifo";
    int policy;

    if (strcmp(mode, "fifo") == 0 || strcmp(mode, "prio") == 0) {
        policy = SCHED_FIFO;
    } else if (strcmp(mode, "rr") == 0) {
        policy = SCHED_RR;
    } else {
        fprintf(stderr, "Usage: sudo %s [-s] fifo|rr|prio\n", prog);
        return 2;
    }
    if (strcmp(mode, "prio") == 0) {
        jobs = prio_jobs;
        n = (int)(sizeof(prio_jobs) / sizeof(prio_jobs[0]));
    } else {
        jobs = srtf_jobs;
        n = (int)(sizeof(srtf_jobs) / sizeof(srtf_jobs[0]));
    }
    if (geteuid() != 0) {
        fprintf(stderr, "Real-time policies need root: sudo %s %s\n", prog, mode);
        return 1;
    }
    if (sysconf(_SC_NPROCESSORS_ONLN) < 2) {
        fprintf(stderr, "Needs at least 2 CPUs: the parent runs on CPU 1, the jobs on CPU 0.\n");
        return 1;
    }

    /* Make the shared memory before forking, so every child gets the same mapping. */
    struct shared *sh = mmap(NULL, sizeof(*sh), PROT_READ | PROT_WRITE,
                             MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (sh == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    memset(sh, 0, sizeof(*sh));
    start = &sh->start;

    /* The parent stays off CPU 0 so it can start each arrival on time. */
    cpu_set_t cpu1;
    CPU_ZERO(&cpu1);
    CPU_SET(1, &cpu1);
    if (sched_setaffinity(0, sizeof(cpu1), &cpu1) < 0) {
        perror("sched_setaffinity");
        return 1;
    }

    printf("Policy %s (%s), 1 unit = %d ms\n", mode,
           policy == SCHED_RR ? "SCHED_RR" : "SCHED_FIFO", unit_ms);
    if (policy == SCHED_RR) {
        printf("Kernel round robin quantum: %ld ms (/proc/sys/kernel/sched_rr_timeslice_ms)\n",
               read_proc_long("/proc/sys/kernel/sched_rr_timeslice_ms"));
    }
    if (strcmp(mode, "prio") == 0) {
        for (int i = 0; i < n; i++) {
            printf("  %s burst %2d, slide priority %d = Linux real-time priority %d\n",
                   jobs[i].name, jobs[i].burst, jobs[i].priority, jobs[i].rt_priority);
        }
    }
    if (read_proc_long("/proc/sys/kernel/sched_rt_runtime_us") != -1) {
        printf("Note: real-time throttling is on (95%%), so expect a 50 ms gap every second.\n");
    }
    fflush(stdout);

    /*
     * Create every child up front. Each sets itself up and then waits on its own "go"
     * pipe; the parent counts one byte on the "ready" pipe per child that is set up.
     */
    int ready[2];
    int go[8][2];
    if (pipe(ready) < 0) {
        perror("pipe");
        return 1;
    }
    for (int i = 0; i < n; i++) {
        if (pipe(go[i]) < 0) {
            perror("pipe");
            return 1;
        }
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return 1;
        }
        if (pid == 0) {
            worker(sh, i, policy, jobs[i].rt_priority, ready[1], go[i][0]);
        }
    }
    for (int i = 0; i < n; i++) {
        char byte;
        if (read(ready[0], &byte, 1) != 1) {
            fprintf(stderr, "a child failed to start\n");
            return 1;
        }
    }

    /*
     * Decide the order to release the jobs: by arrival time, and among jobs that
     * arrive together, highest priority first. Released the other way around, a
     * lower-priority job would get the CPU for the moment before the next release.
     * (A simple insertion sort; there are at most 5 jobs.)
     */
    int order[8];
    for (int i = 0; i < n; i++) {
        int j = i;
        while (j > 0 && (jobs[order[j - 1]].arrival > jobs[i].arrival ||
                         (jobs[order[j - 1]].arrival == jobs[i].arrival &&
                          jobs[order[j - 1]].rt_priority < jobs[i].rt_priority))) {
            order[j] = order[j - 1];
            j--;
        }
        order[j] = i;
    }

    /* Time 0. Release each child at its arrival time, then wait for all of them. */
    clock_gettime(CLOCK_MONOTONIC, start);
    for (int k = 0; k < n; k++) {
        int i = order[k];
        long wait_us = (long)jobs[i].arrival * unit_ms * 1000 - now_us();
        if (wait_us > 0) {
            usleep((useconds_t)wait_us);
        }
        if (write(go[i][1], "g", 1) != 1) {
            perror("write");
            return 1;
        }
    }
    while (wait(NULL) > 0) {
    }

    /* Merge back-to-back stretches of the same job, then print the timeline. */
    int nsegs = sh->nsegs < MAX_SEGS ? sh->nsegs : MAX_SEGS;
    qsort(sh->segs, (size_t)nsegs, sizeof(sh->segs[0]), by_start);

    /* Each job's first stretch is when it first ran, for its response time. */
    long first_us[8];
    for (int i = 0; i < n; i++) {
        first_us[i] = -1;
    }
    for (int k = 0; k < nsegs; k++) {
        if (first_us[sh->segs[k].job] < 0) {
            first_us[sh->segs[k].job] = sh->segs[k].start_us;
        }
    }
    printf("\nMeasured timeline (who the kernel actually ran on CPU 0):\n");
    for (int k = 0; k < nsegs;) {
        int j = k;
        while (j + 1 < nsegs && sh->segs[j + 1].job == sh->segs[k].job &&
               sh->segs[j + 1].start_us - sh->segs[j].end_us < 2000) {
            j++;
        }
        printf("  %6.2f - %6.2f  %s\n", units(sh->segs[k].start_us), units(sh->segs[j].end_us),
               jobs[sh->segs[k].job].name);
        k = j + 1;
    }

    /*
     * The same measures as the slides:
     *   turnaround = completion - arrival
     *   waiting    = turnaround - burst
     *   response   = first run - arrival
     */
    double total_turnaround = 0.0;
    double total_wait = 0.0;
    double total_response = 0.0;
    printf("\n%-4s %-7s %-5s %-10s %-10s %-7s %-8s\n", "Proc", "Arrival", "Burst", "Completion",
           "Turnaround", "Wait", "Response");
    for (int i = 0; i < n; i++) {
        double completion = units(sh->done_us[i]);
        double turnaround = completion - jobs[i].arrival;
        double wait = turnaround - jobs[i].burst;
        double response = units(first_us[i]) - jobs[i].arrival;
        printf("%-4s %-7d %-5d %-10.2f %-10.2f %-7.2f %-8.2f\n", jobs[i].name, jobs[i].arrival,
               jobs[i].burst, completion, turnaround, wait, response);
        total_turnaround += turnaround;
        total_wait += wait;
        total_response += response;
    }
    printf("\nAverage turnaround time = %.2f units (measured)\n", total_turnaround / n);
    printf("Average waiting time    = %.2f units (measured)\n", total_wait / n);
    printf("Average response time   = %.2f units (measured)\n", total_response / n);
    return 0;
}
