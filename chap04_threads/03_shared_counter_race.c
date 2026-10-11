/*
 * Chapter 4: Threads
 * Demo 03: Shared counter race
 *
 * What's going on
 *   All four threads add 1 to the same global variable, 100,000 times each, so the
 *   answer should be 400,000. But shared_counter++ isn't one step. The CPU loads the
 *   value into a register, adds 1, and stores it back. If two threads both load 41,
 *   both add 1, and both store 42, one of the two increments is lost. This is a race
 *   condition.
 *
 * Build
 *   make
 *   The Makefile builds without optimization. With gcc -O2 the compiler replaces the
 *   whole loop with one addition, so the bug is still there but almost never shows.
 *
 * Run
 *   ./03_shared_counter_race
 *   Run it several times.
 *
 * What to look for
 *   "Actual value" is less than 400,000, and it's different every run. On the course
 *   VM it usually lands somewhere around 300,000.
 *
 * Other ways to see it
 *   - Change THREAD_COUNT to 1. The answer is always right, because nothing else
 *     touches the counter.
 *   - Demo 04 is this same program with a mutex added.
 */
#include <pthread.h> /* pthread_t, pthread_create(), pthread_join() */
#include <stdio.h>   /* printf(), fprintf() */
#include <string.h>  /* strerror() */

enum {
    THREAD_COUNT = 4,
    INCREMENTS_PER_THREAD = 100000
};

/* A global, so every thread in the process sees the same variable. */
static long shared_counter = 0;

static void *increment_worker(void *arg)
{
    (void)arg; /* this thread needs no argument; (void) says we're ignoring it */

    for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
        shared_counter++; /* load, add 1, store: another thread can run in between */
    }

    return NULL;
}

int main(void)
{
    pthread_t threads[THREAD_COUNT];
    int rc;

    /* Start every thread before joining any, so they all run at the same time. */
    for (int i = 0; i < THREAD_COUNT; i++) {
        rc = pthread_create(&threads[i], NULL, increment_worker, NULL);
        if (rc != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(rc));
            return 1;
        }
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        rc = pthread_join(threads[i], NULL);
        if (rc != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(rc));
            return 1;
        }
    }

    /* All threads are done, so main can read the counter safely now. */
    printf("Expected value = %d\n", THREAD_COUNT * INCREMENTS_PER_THREAD);
    printf("Actual value   = %ld\n", shared_counter);
    return 0;
}
