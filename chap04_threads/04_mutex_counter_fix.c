/*
 * Chapter 4: Threads
 * Demo 04: Fixing the shared-counter race with a mutex
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    THREAD_COUNT = 4,
    INCREMENTS_PER_THREAD = 100000
};

static long shared_counter = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void *increment_worker(void *arg)
{
    (void)arg;

    for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
        if (pthread_mutex_lock(&lock) != 0) {
            perror("pthread_mutex_lock");
            return NULL;
        }

        shared_counter++;

        if (pthread_mutex_unlock(&lock) != 0) {
            perror("pthread_mutex_unlock");
            return NULL;
        }
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

    if (pthread_mutex_destroy(&lock) != 0) {
        perror("pthread_mutex_destroy");
        return 1;
    }

    return 0;
}
