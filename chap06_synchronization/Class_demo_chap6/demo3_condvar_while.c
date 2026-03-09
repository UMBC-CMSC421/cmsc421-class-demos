// demo3_condvar_while.c
// Correct pthread condition variable usage: predicate checked in a while-loop.
//
// Build: gcc -O2 -pthread demo3_condvar_while.c -o demo3
// Run: ./demo3 123

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cv  = PTHREAD_COND_INITIALIZER;

static int ready = 0;       // predicate: "work available?"
static int payload = 0;

static void* consumer(void* arg) {
    (void)arg;

    pthread_mutex_lock(&mtx);

    // MUST be while, not if: protects against spurious wakeups and missed conditions.
    while (ready == 0) {
        // Atomically: unlock mtx, sleep; then on wake, re-lock mtx before returning.
        pthread_cond_wait(&cv, &mtx);
    }

    // Now predicate holds under the mutex
    printf("[consumer] got payload=%d\n", payload);

    pthread_mutex_unlock(&mtx);
    return NULL;
}

static void* producer(void* arg) {
    int value = *(int*)arg;

    // Simulate doing work first - five seconds
    sleep(5);

    pthread_mutex_lock(&mtx);
    payload = value;
    ready = 1;

    // Signal AFTER making predicate true (while holding the lock is conventional/safe)
    pthread_cond_signal(&cv);

    pthread_mutex_unlock(&mtx);
    return NULL;
}

int main(int argc, char** argv) {
    int value = 42;
    if (argc >= 2) value = atoi(argv[1]);

    pthread_t tc, tp;

    pthread_create(&tc, NULL, consumer, NULL);
    pthread_create(&tp, NULL, producer, &value);

    pthread_join(tc, NULL);
    pthread_join(tp, NULL);

    return 0;
}
