/*
 * Chapter 6: Synchronization Tools
 * Demo 04: Waiting with a condition variable
 *
 * What this demo shows:
 * - A thread can sleep until shared state becomes true.
 * - The waiting loop must re-check the predicate after wakeup.
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int ready = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;

static void *waiting_thread(void *arg)
{
    (void)arg;

    pthread_mutex_lock(&lock);
    while (!ready) {
        printf("[waiter ] not ready yet; waiting...\n");
        pthread_cond_wait(&condition, &lock);
    }
    printf("[waiter ] observed ready=1; continuing\n");
    pthread_mutex_unlock(&lock);

    return NULL;
}

static void *signaling_thread(void *arg)
{
    (void)arg;

    sleep(2);

    pthread_mutex_lock(&lock);
    ready = 1;
    printf("[signal ] set ready=1 and will signal\n");
    pthread_cond_signal(&condition);
    pthread_mutex_unlock(&lock);

    return NULL;
}

int main(void)
{
    pthread_t waiter;
    pthread_t signaler;

    if (pthread_create(&waiter, NULL, waiting_thread, NULL) != 0) {
        perror("pthread_create waiter");
        return 1;
    }

    if (pthread_create(&signaler, NULL, signaling_thread, NULL) != 0) {
        perror("pthread_create signaler");
        return 1;
    }

    pthread_join(waiter, NULL);
    pthread_join(signaler, NULL);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&condition);
    return 0;
}
