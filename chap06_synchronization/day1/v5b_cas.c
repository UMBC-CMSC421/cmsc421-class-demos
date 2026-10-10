// v5b_cas.c: the same kind of spinlock, built on compare-and-swap (Figure 6.4.4).
// DAY 1 SLIDES 43-44. lock is 0 when it's free. Swapping it from 0 to 1 claims it.
// compare_and_swap() below works like the textbook's: if *value equals expected,
// set it to new_value, and either way return the old value. All in one step.
// EXPECT: 2,000,000. Same downsides as v5: it spins, and nobody's guaranteed a turn.
// BUILD: gcc -O2 -pthread v5b_cas.c -o v5b_cas
// RUN:   ./v5b_cas
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#define N 1000000
volatile long counter = 0;            // volatile: every access really goes to memory; not a lock (see v1_race.c)
atomic_int lock = 0;                  // 0: free, 1: held

int compare_and_swap(atomic_int *value, int expected, int new_value) {
    // C11 returns true or false, but the textbook returns the old value. So we
    // return expected: still 0 if the swap worked, or the 1 that C11 saw if it didn't.
    atomic_compare_exchange_strong(value, &expected, new_value);
    return expected;                  // the old value, either way
}

void *worker(void *arg) {
    for (int k = 0; k < N; k++) {
        while (compare_and_swap(&lock, 0, 1) != 0)  // entry: spin until I swap 0 to 1
            ;
        counter++;                                  // critical section
        atomic_store(&lock, 0);                     // exit: lock = 0, free again
    }
    return NULL;
}

int main(void) {
    pthread_t a, b;
    pthread_create(&a, NULL, worker, NULL);
    pthread_create(&b, NULL, worker, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("counter = %ld (expected %d)\n", counter, 2 * N);
    return 0;
}
