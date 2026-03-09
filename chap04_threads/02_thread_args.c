/*
 * Chapter 4: Threads
 * Demo 02: Passing arguments to threads
 *
 * What this demo shows:
 * - Use a struct when a thread needs multiple input values.
 * - Each thread receives its own argument block.
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

struct task {
    int index;
    int value;
};

static void *square_worker(void *arg)
{
    const struct task *t = arg;
    printf("[thread %d] value=%d square=%d\n", t->index, t->value, t->value * t->value);
    return NULL;
}

int main(void)
{
    enum { THREAD_COUNT = 4 };
    pthread_t threads[THREAD_COUNT];
    struct task tasks[THREAD_COUNT];

    for (int i = 0; i < THREAD_COUNT; i++) {
        tasks[i].index = i;
        tasks[i].value = i + 2;

        if (pthread_create(&threads[i], NULL, square_worker, &tasks[i]) != 0) {
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

    return 0;
}
