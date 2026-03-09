/*
 * Chapter 6: Synchronization Tools
 * Demo 05: Deliberate deadlock
 *
 * What this demo shows:
 * - Deadlock can occur when threads acquire locks in inconsistent order.
 * - This program intentionally hangs.
 *
 * To stop it:
 *   Ctrl-C
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static pthread_mutex_t lock_a = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t lock_b = PTHREAD_MUTEX_INITIALIZER;

static void *thread_one(void *arg)
{
    (void)arg;

    pthread_mutex_lock(&lock_a);
    printf("[thread 1] acquired lock_a\n");
    sleep(1);

    printf("[thread 1] waiting for lock_b\n");
    pthread_mutex_lock(&lock_b);
    printf("[thread 1] acquired lock_b\n");

    pthread_mutex_unlock(&lock_b);
    pthread_mutex_unlock(&lock_a);
    return NULL;
}

static void *thread_two(void *arg)
{
    (void)arg;

    pthread_mutex_lock(&lock_b);
    printf("[thread 2] acquired lock_b\n");
    sleep(1);

    printf("[thread 2] waiting for lock_a\n");
    pthread_mutex_lock(&lock_a);
    printf("[thread 2] acquired lock_a\n");

    pthread_mutex_unlock(&lock_a);
    pthread_mutex_unlock(&lock_b);
    return NULL;
}

int main(void)
{
    pthread_t t1;
    pthread_t t2;

    if (pthread_create(&t1, NULL, thread_one, NULL) != 0) {
        perror("pthread_create t1");
        return 1;
    }

    if (pthread_create(&t2, NULL, thread_two, NULL) != 0) {
        perror("pthread_create t2");
        return 1;
    }

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return 0;
}
