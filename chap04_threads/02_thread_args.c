/*
 * Chapter 4: Threads
 * Demo 02: Passing arguments to threads
 *
 * What's going on
 *   A thread function gets exactly one argument, a void *. When a thread needs more
 *   than one value, put the values in a struct and pass the struct's address.
 *
 *   Each thread here gets its own struct, &tasks[i]. Passing &i would be a bug: there
 *   is only one i, main keeps changing it as the loop runs, and a thread that reads it
 *   late sees whatever value it has by then.
 *
 * Build
 *   make
 *
 * Run
 *   ./02_thread_args
 *
 * What to look for
 *   Four lines, one per thread, each with the right square. The order can change from
 *   run to run, because the threads run at the same time and any of them can print
 *   first.
 */
#include <pthread.h> /* pthread_t, pthread_create(), pthread_join() */
#include <stdio.h>   /* printf(), fprintf() */
#include <string.h>  /* strerror() */

/* Everything one thread needs to know, packed into one struct. */
struct task {
    int index; /* which thread this is, 0 to 3 */
    int value; /* the number this thread squares */
};

static void *square_worker(void *arg)
{
    const struct task *t = arg; /* turn the void * back into the struct main passed */
    printf("[thread %d] value=%d square=%d\n", t->index, t->value, t->value * t->value);
    return NULL;
}

int main(void)
{
    enum { THREAD_COUNT = 4 };          /* a named constant usable as an array size */
    pthread_t threads[THREAD_COUNT];    /* one thread ID per thread */
    struct task tasks[THREAD_COUNT];    /* one argument struct per thread */
    int rc;

    /* Start all four threads. Each gets the address of its own tasks[i]. */
    for (int i = 0; i < THREAD_COUNT; i++) {
        tasks[i].index = i;
        tasks[i].value = i + 2;

        rc = pthread_create(&threads[i], NULL, square_worker, &tasks[i]);
        if (rc != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(rc));
            return 1;
        }
    }

    /*
     * Wait for all four. tasks[] lives in main's stack frame, so main must not return
     * while a thread might still be reading its struct.
     */
    for (int i = 0; i < THREAD_COUNT; i++) {
        rc = pthread_join(threads[i], NULL);
        if (rc != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(rc));
            return 1;
        }
    }

    return 0;
}
