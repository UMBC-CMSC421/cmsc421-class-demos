/*
 * Chapter 6: Synchronization Tools
 * Demo 02: Mutual exclusion with a mutex
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    THREAD_COUNT = 4,
    LOOP_COUNT = 50000
};

static int counter = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void *worker(void *arg)
{
    (void)arg;

    for (int i = 0; i < LOOP_COUNT; i++) {
        if (pthread_mutex_lock(&lock) != 0) {
            perror("pthread_mutex_lock");
            return NULL;
        }

        counter++;

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
        if (pthread_create(&threads[i], NULL, worker, NULL) != 0) {
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

    printf("Expected = %d\n", THREAD_COUNT * LOOP_COUNT);
    printf("Actual   = %d\n", counter);

    if (pthread_mutex_destroy(&lock) != 0) {
        perror("pthread_mutex_destroy");
        return 1;
    }

    return 0;
}
