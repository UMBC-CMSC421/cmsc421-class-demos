// cas_vs_mutex.c: optimistic (compare-and-swap) vs. pessimistic (mutex).
// Slides: Evaluating the Tools: Contention; Optimistic vs. Pessimistic in Code.
// Every thread adds 1 to a shared counter, 2,000,000 times.
//   cas:   try to swap in the new value, and retry if someone beat you to it
//   mutex: lock, add, unlock
// EXPECT: failed CAS goes from 0 at 1 thread to millions at 8. Which one's
// faster depends on your machine. With enough CPUs the mutex wins at 4 or 8
// threads, which is the textbook's high-contention case.
// BUILD: gcc -O2 -Wall -pthread cas_vs_mutex.c -o cas_vs_mutex
// RUN:   for t in 1 2 4 8; do ./cas_vs_mutex cas $t; ./cas_vs_mutex mutex $t; done
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N 2000000                       // increments per thread
atomic_long counter;
long plain_counter;
atomic_long failed;                     // CAS attempts that lost a race
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int use_cas;

long add_cas(void) {                    // optimistic: returns failed tries
    long fails = 0;
    long old = atomic_load(&counter);
    while (!atomic_compare_exchange_weak(&counter, &old, old + 1))
        fails++;                        // lost the race; old was reloaded
    return fails;
}

void add_mutex(void) {                  // pessimistic: lock first
    pthread_mutex_lock(&m);
    plain_counter++;
    pthread_mutex_unlock(&m);
}

void *worker(void *arg) {
    long my_failed = 0;
    for (int k = 0; k < N; k++) {
        if (use_cas) my_failed += add_cas();
        else         add_mutex();
    }
    atomic_fetch_add(&failed, my_failed);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 2 || (strcmp(argv[1], "cas") && strcmp(argv[1], "mutex"))) {
        fprintf(stderr, "usage: %s cas|mutex [threads]\n", argv[0]); return 1;
    }
    use_cas = strcmp(argv[1], "cas") == 0;
    int n = argc > 2 ? atoi(argv[2]) : 2;
    if (n < 1 || n > 64) n = 2;
    pthread_t t[64];
    struct timespec a, b;
    clock_gettime(CLOCK_MONOTONIC, &a);
    for (int i = 0; i < n; i++) pthread_create(&t[i], NULL, worker, NULL);
    for (int i = 0; i < n; i++) pthread_join(t[i], NULL);
    clock_gettime(CLOCK_MONOTONIC, &b);
    long total = use_cas ? atomic_load(&counter) : plain_counter;
    if (total != (long)n * N) printf("WRONG COUNT: %ld\n", total);
    printf("%-5s %d thread%s: %.2f s", argv[1], n, n == 1 ? " " : "s",
           (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) / 1e9);
    if (use_cas) printf(", failed CAS: %ld", atomic_load(&failed));
    printf("\n");
    return 0;
}
