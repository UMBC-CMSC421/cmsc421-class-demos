// v6_atomic.c: make the counter itself atomic.
// DAY 1 SLIDES 45-46. No lock this time.
// atomic_fetch_add does the read, add, and write in one step, so nothing gets lost.
// EXPECT: 2,000,000. Great for one variable. v7 shows where it stops working.
// BUILD: gcc -O2 -pthread v6_atomic.c -o v6_atomic
// RUN:   ./v6_atomic
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#define N 1000000
atomic_long counter = 0;              // atomic: every update is indivisible

void *worker(void *arg) {
    for (int k = 0; k < N; k++)
        atomic_fetch_add(&counter, 1); // one indivisible read-modify-write
    return NULL;
}

int main(void) {
    pthread_t a, b;
    pthread_create(&a, NULL, worker, NULL);
    pthread_create(&b, NULL, worker, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("counter = %ld (expected %d)\n", (long)counter, 2 * N);
    return 0;
}
