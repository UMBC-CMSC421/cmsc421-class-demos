// rwlock.c: lots of readers at once, or one writer, never both.
// Slides: A Reader-Writer Lock: What It Does; Does a Reader-Writer Lock Meet Them?;
//         A Reader-Writer Lock in Code.
// 4 readers read a shared table over and over, and 1 writer updates it now and then.
//   mutex   a plain mutex: one at a time, readers too
//   rw      glibc's reader-writer lock (it favors readers)
//   writer  the same lock, set to favor the writer
// EXPECT: mutex: 1 reader at a time, and the slowest. rw: 4 readers at once and
// way faster, but the writer waits hundreds of ms because readers keep cutting
// in. writer: 4 at once, and the writer waits only a few ms. Rule breaks: always 0.
// BUILD: gcc -O2 -Wall -pthread rwlock.c -o rwlock
// RUN:   ./rwlock mutex      ./rwlock rw      ./rwlock writer
#define _GNU_SOURCE // for pthread_rwlockattr_setkind_np
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define READERS 4
#define READS 200 // reads per reader
#define WRITES 20 // writes by the writer
int use_rw;
pthread_rwlock_t rw;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
int table[100];                    // the shared data
atomic_int readers_in, writers_in; // who is inside right now
atomic_int most_readers, rule_breaks;
double max_wait; // written only by the writer thread

void rd_lock(void)
{
    if (use_rw)
        pthread_rwlock_rdlock(&rw); // read mode: shared
    else
        pthread_mutex_lock(&mtx);
}
void wr_lock(void)
{
    if (use_rw)
        pthread_rwlock_wrlock(&rw); // write mode: exclusive
    else
        pthread_mutex_lock(&mtx);
}
void unlock(void)
{
    if (use_rw)
        pthread_rwlock_unlock(&rw); // one unlock for both modes
    else
        pthread_mutex_unlock(&mtx);
}

static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

// Called with the lock held. The counters only check the rules.
// usleep stands in for a slow read or write, such as one that waits on a device.
void read_table(void)
{
    int now_in = atomic_fetch_add(&readers_in, 1) + 1;
    if (atomic_load(&writers_in) > 0)
        atomic_fetch_add(&rule_breaks, 1);
    int most = atomic_load(&most_readers);
    while (now_in > most && !atomic_compare_exchange_weak(&most_readers, &most, now_in))
        ;
    volatile int sum = 0;
    for (int k = 0; k < 100; k++)
        sum += table[k];
    usleep(1000); // the read takes 1 ms
    atomic_fetch_sub(&readers_in, 1);
}
void write_table(void)
{
    atomic_fetch_add(&writers_in, 1);
    if (atomic_load(&readers_in) > 0 || atomic_load(&writers_in) > 1)
        atomic_fetch_add(&rule_breaks, 1);
    for (int k = 0; k < 100; k++)
        table[k]++;
    usleep(1000); // the write takes 1 ms
    atomic_fetch_sub(&writers_in, 1);
}

void *reader(void *arg)
{
    for (int i = 0; i < READS; i++)
    {
        rd_lock();
        read_table();
        unlock();
    }
    return NULL;
}

void *writer(void *arg)
{
    for (int i = 0; i < WRITES; i++)
    {
        double asked = now();
        wr_lock();
        double waited = now() - asked; // how long until the writer got in
        if (waited > max_wait)
            max_wait = waited;
        write_table();
        unlock();
        usleep(5000); // writes are rare
    }
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc < 2 || (strcmp(argv[1], "mutex") && strcmp(argv[1], "rw") && strcmp(argv[1], "writer")))
    {
        fprintf(stderr, "usage: %s mutex|rw|writer\n", argv[0]);
        return 1;
    }
    use_rw = strcmp(argv[1], "mutex") != 0;
    pthread_rwlockattr_t attr;
    pthread_rwlockattr_init(&attr);
    if (strcmp(argv[1], "writer") == 0)
        pthread_rwlockattr_setkind_np(&attr,
                                      PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    pthread_rwlock_init(&rw, &attr); // "rw" keeps glibc's default: prefer readers

    pthread_t r[READERS], w;
    double start = now();
    for (int i = 0; i < READERS; i++)
        pthread_create(&r[i], NULL, reader, NULL);
    pthread_create(&w, NULL, writer, NULL);
    for (int i = 0; i < READERS; i++)
        pthread_join(r[i], NULL);
    pthread_join(w, NULL);
    printf("%-6s readers at once: %d | writer's max wait: %3.0f ms | %.2f s | rule breaks: %d\n",
           argv[1], atomic_load(&most_readers), max_wait * 1e3, now() - start,
           atomic_load(&rule_breaks));
    return 0;
}
