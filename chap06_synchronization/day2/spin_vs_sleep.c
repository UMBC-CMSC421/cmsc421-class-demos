// spin_vs_sleep.c: the same work under a spinlock, then under a mutex.
// Slides: Spinlock or Sleeping Lock?; Spinlock vs. Mutex in Code.
// While one thread holds the lock, the rest wait. With spin, they loop on their
// CPU. With mutex, they go to sleep and free up the CPU.
// EXPECT: at 2 threads, spin is about as fast or faster. At 4 threads per CPU,
// the mutex finishes first and uses way less CPU time. Check the last line.
// BUILD: gcc -O2 -Wall -pthread spin_vs_sleep.c -o spin_vs_sleep
// RUN:   ./spin_vs_sleep spin 2      2 threads, spinlock
//        ./spin_vs_sleep mutex 2     2 threads, mutex
//        ./spin_vs_sleep spin $((4 * $(nproc)))     4 threads per CPU
//        ./spin_vs_sleep mutex $((4 * $(nproc)))    (24 threads on a 6-CPU VM)
#define _GNU_SOURCE                       // RUSAGE_THREAD (Linux)
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

#define ROUNDS 20000                      // times each thread takes the lock
#define WORK   2000                       // work done while holding the lock

int use_spin;                             // 1 = spinlock, 0 = mutex
pthread_spinlock_t spin;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
volatile long shared = 0;

// Measurement code, at the bottom of the file. It records and prints the
// numbers; it is not part of the locking.
struct stats { double cpu_s; long slept, preempted; };
void   record_thread(struct stats *me);
double work_alone(int n);
void   start_clocks(void);
void   stop_clocks(void);
void   print_report(const char *mode, int n, double work_s, struct stats *st);

// ---------------------------------------------------------------------------
// The demo: worker and main
// ---------------------------------------------------------------------------

void *worker(void *arg) {
    for (int r = 0; r < ROUNDS; r++) {
        if (use_spin) pthread_spin_lock(&spin);   // busy? loop on the CPU
        else          pthread_mutex_lock(&mtx);   // busy? sleep in the kernel
        for (int w = 0; w < WORK; w++)            // critical section:
            shared++;                             // hold the lock a while
        if (use_spin) pthread_spin_unlock(&spin);
        else          pthread_mutex_unlock(&mtx);
    }
    record_thread(arg);                   // save this thread's CPU time
    return NULL;
}

int main(int argc, char **argv) {
    // 1. Which lock, and how many threads?
    if (argc < 2 || (strcmp(argv[1], "spin") && strcmp(argv[1], "mutex"))) {
        fprintf(stderr, "usage: %s spin|mutex [threads]\n", argv[0]); return 1;
    }
    use_spin = strcmp(argv[1], "spin") == 0;
    int n = argc > 2 ? atoi(argv[2]) : 8;
    if (n < 1 || n > 64) n = 8;
    pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE);

    // 2. How much CPU does the work need with no lock and no other threads?
    double work_s = work_alone(n);

    // 3. Run the threads.
    pthread_t t[64];
    struct stats st[64];
    start_clocks();
    for (int i = 0; i < n; i++) pthread_create(&t[i], NULL, worker, &st[i]);
    for (int i = 0; i < n; i++) pthread_join(t[i], NULL);
    stop_clocks();

    // 4. Print what happened.
    print_report(argv[1], n, work_s, st);
    return 0;
}

// ---------------------------------------------------------------------------
// Measurement code: clocks, counters, and printing
// ---------------------------------------------------------------------------

static double wall_start, cpu_start, wall, cpu;

static double now(void) {                 // wall-clock time
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static double thread_cpu(void) {          // CPU time of the calling thread
    struct timespec ts; clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static double process_cpu(void) {         // CPU time of all threads together
    struct rusage ru; getrusage(RUSAGE_SELF, &ru);
    return ru.ru_utime.tv_sec + ru.ru_utime.tv_usec / 1e6
         + ru.ru_stime.tv_sec + ru.ru_stime.tv_usec / 1e6;
}

// Called by each thread when it finishes.
void record_thread(struct stats *me) {
    me->cpu_s = thread_cpu();
    struct rusage ru; getrusage(RUSAGE_THREAD, &ru);
    me->slept = ru.ru_nvcsw;              // voluntary: gave up the CPU
    me->preempted = ru.ru_nivcsw;         // involuntary: CPU taken away
}

// Do one thread's share of the work alone, then scale it to n threads.
double work_alone(int n) {
    double c0 = thread_cpu();
    for (int r = 0; r < ROUNDS; r++)
        for (int w = 0; w < WORK; w++)
            shared++;
    shared = 0;
    return (thread_cpu() - c0) * n;
}

void start_clocks(void) { cpu_start = process_cpu(); wall_start = now(); }
void stop_clocks(void)  { wall = now() - wall_start; cpu = process_cpu() - cpu_start; }

void print_report(const char *mode, int n, double work_s, struct stats *st) {
    long cpus = sysconf(_SC_NPROCESSORS_ONLN);
    printf("%s lock: %d threads on %ld CPUs\n\n", mode, n, cpus);

    printf("thread  CPU time   slept  preempted\n");
    struct stats sum = {0};
    for (int i = 0; i < n; i++) {
        if (n <= 8 || i < 4)              // many threads: show the first 4
            printf("%6d  %6.3f s  %6ld  %9ld\n",
                   i + 1, st[i].cpu_s, st[i].slept, st[i].preempted);
        sum.cpu_s += st[i].cpu_s;
        sum.slept += st[i].slept;
        sum.preempted += st[i].preempted;
    }
    if (n > 8) printf("   ...  (%d more threads)\n", n - 4);
    printf(" total  %6.3f s  %6ld  %9ld\n", sum.cpu_s, sum.slept, sum.preempted);
    printf("slept:     the thread gave up its CPU (went to sleep)\n"
           "preempted: the scheduler took its CPU away\n\n");

    printf("shared = %ld (expected %ld)\n\n", shared, (long)n * ROUNDS * WORK);

    printf("wall time            %6.3f s   start to finish\n", wall);
    printf("CPU time             %6.3f s   all threads added up: %.1f of %ld CPUs busy\n",
           cpu, cpu / wall, cpus);
    printf("  doing the work     %6.3f s   same work, run alone with no lock\n", work_s);
    printf("  waiting            %6.3f s   %s\n\n", cpu - work_s,
           use_spin ? "spinning on the CPU" : "going to sleep and waking up");

    printf("%-5s %d threads: wall %.2f s, CPU %.2f s\n", mode, n, wall, cpu);
}
