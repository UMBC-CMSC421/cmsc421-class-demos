// v5_tas.c: a spinlock built on the test-and-set instruction.
// DAY 1 SLIDES 40-42. counter++ is the same; now there's a lock around it.
// atomic_flag_test_and_set() is the textbook's test_and_set(): it sets the flag
// to true and hands back the old value, all in one step.
// Got false back? The lock's yours. Got true? Someone has it, so keep trying.
// EXPECT: 2,000,000. But waiting threads burn CPU, and an unlucky one can lose every time.
// BUILD: gcc -O2 -pthread v5_tas.c -o v5_tas
// RUN:   ./v5_tas
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#define N 1000000
volatile long counter = 0;            // volatile: every access really goes to memory; not a lock (see v1_race.c)
atomic_flag lock = ATOMIC_FLAG_INIT;  // false: nobody holds the lock

void *worker(void *arg) {
    for (int k = 0; k < N; k++) {
        while (atomic_flag_test_and_set(&lock))  // entry: spin until old value was false
            ;
        counter++;                               // critical section
        atomic_flag_clear(&lock);                // exit: lock = false
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
