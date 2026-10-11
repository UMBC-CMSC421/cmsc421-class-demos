/*
 * Chapter 4: Threads
 * Demo 04: Fixing the shared-counter race with a mutex
 *
 * What's going on
 *   This is Demo 03 with two new lines around shared_counter++: a lock and an unlock.
 *   A mutex ("mutual exclusion") lets only one thread at a time hold it. The code
 *   between lock and unlock is the critical section. A thread that calls lock while
 *   another thread holds the mutex waits until it's unlocked. So no two threads can
 *   be in the middle of the load, add, store at once, and no increment is lost.
 *
 *   The critical section holds only the shared update. Everything else stays outside
 *   it, so the threads wait for each other as little as possible.
 *
 * Build
 *   make
 *
 * Run
 *   ./04_mutex_counter_fix
 *
 * What to look for
 *   "Actual value" is 400000 every run.
 *
 * Other ways to see it
 *   - Time both demos: time ./03_shared_counter_race, then time ./04_mutex_counter_fix.
 *     The mutex version is slower, because the threads now take turns.
 */
#include <pthread.h> /* pthread_t, pthread_mutex_t, pthread_create(), pthread_join() */
#include <stdio.h>   /* printf(), fprintf() */
#include <string.h>  /* strerror() */

enum {
    THREAD_COUNT = 4,
    INCREMENTS_PER_THREAD = 100000
};

static long shared_counter = 0;

/* One mutex guards shared_counter. PTHREAD_MUTEX_INITIALIZER sets it up unlocked. */
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void *increment_worker(void *arg)
{
    (void)arg;

    for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
        pthread_mutex_lock(&lock);   /* wait here if another thread holds the lock */
        shared_counter++;            /* only one thread at a time runs this line */
        pthread_mutex_unlock(&lock); /* let the next waiting thread in */
    }

    return NULL;
}

int main(void)
{
    pthread_t threads[THREAD_COUNT];
    int rc;

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

    printf("Expected value = %d\n", THREAD_COUNT * INCREMENTS_PER_THREAD);
    printf("Actual value   = %ld\n", shared_counter);

    return 0;
}
