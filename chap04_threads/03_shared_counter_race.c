/*
 * Chapter 4: Threads
 * Demo 03: Shared counter race
 *
 * What this demo shows:
 * - Threads share memory.
 * - Incrementing a shared variable is not automatically atomic.
 * - Lost updates can occur when multiple threads interleave badly.
 *
 * Run this several times. The final answer may be smaller than expected.
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    THREAD_COUNT = 4,
    INCREMENTS_PER_THREAD = 100000
};

static long shared_counter = 0;

static void *increment_worker(void *arg)
{
    (void)arg;

    for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
        shared_counter++;
    }

    return NULL;
}

int main(void)
{
    pthread_t threads[THREAD_COUNT];

    for (int i = 0; i < THREAD_COUNT; i++) {
        if (pthread_create(&threads[i], NULL, increment_worker, NULL) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            return 1;
        }
    }

    printf("Expected value = %d\n", THREAD_COUNT * INCREMENTS_PER_THREAD);
    printf("Actual value   = %ld\n", shared_counter);
    return 0;
}
