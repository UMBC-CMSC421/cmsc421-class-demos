/*
 * Chapter 4: Threads
 * Demo 08: Shared memory without a mutex
 *
 * What's going on
 *   shared_values[] is a global, so all three threads can reach every element of it.
 *   But thread i only ever touches shared_values[i]. No element is written by one
 *   thread while another thread uses it, so there's no race, and no mutex is needed.
 *   A mutex is only needed when two threads use the same data and at least one of
 *   them writes it.
 *
 *   After the threads are joined, main forks a child. The child adds 1000 to its own
 *   copy of shared_values[0], and the parent's copy doesn't change.
 *
 * Build
 *   make
 *
 * Run
 *   ./08_shared_array_no_mutex
 *
 * What to look for
 *   All three threads print main's PID, each with its own TID.
 *   The slot addresses are 4 bytes apart: three ints next to each other in one array.
 *   After the joins, main sees 20 20 20, the values the threads wrote.
 *   The child prints 1020, and the parent still prints 20.
 */
#define _GNU_SOURCE     /* needed for gettid() */
#include <pthread.h>    /* pthread_t, pthread_create(), pthread_join() */
#include <stdio.h>      /* printf(), fprintf(), perror(), setbuf() */
#include <string.h>     /* strerror() */
#include <sys/types.h>  /* pid_t */
#include <sys/wait.h>   /* waitpid(), WIFEXITED(), WEXITSTATUS() */
#include <unistd.h>     /* fork(), getpid(), gettid(), _exit() */

enum { THREAD_COUNT = 3 };

/* A global, so it's part of the memory every thread in the process shares. */
static int shared_values[THREAD_COUNT] = {10, 10, 10};

/* Main fills in one of these for each worker. */
struct worker_info {
    int number; /* worker number, for printing */
    int *slot;  /* the one array element this worker may change */
};

static void *change_one_slot(void *arg)
{
    struct worker_info *info = arg;

    /* No mutex needed: every worker got a different slot address. */
    *info->slot += 10;

    printf("[thread %d] PID=%ld TID=%ld slot=%p value=%d\n",
           info->number,
           (long)getpid(),
           (long)gettid(),
           (void *)info->slot,
           *info->slot);

    return NULL;
}

int main(void)
{
    pthread_t threads[THREAD_COUNT];
    struct worker_info info[THREAD_COUNT];
    pid_t child;
    int status;
    int rc;

    /*
     * Turn off output buffering. Otherwise unprinted text sitting in the buffer at
     * fork() would be copied into the child and printed twice.
     */
    setbuf(stdout, NULL);

    printf("[main] PID=%ld TID=%ld starting values:",
           (long)getpid(), (long)gettid());
    for (int i = 0; i < THREAD_COUNT; i++) {
        printf(" %d", shared_values[i]);
    }
    printf("\n");

    /*
     * Create every worker first, so they can run at the same time.
     * Pass &info[i], never &i: there's only one i, and it keeps changing.
     */
    for (int i = 0; i < THREAD_COUNT; i++) {
        info[i].number = i;
        info[i].slot = &shared_values[i];

        rc = pthread_create(&threads[i], NULL, change_one_slot, &info[i]);
        if (rc != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(rc));
            return 1;
        }
    }

    /* Join in a second loop, so all three workers can exist at once. */
    for (int i = 0; i < THREAD_COUNT; i++) {
        rc = pthread_join(threads[i], NULL);
        if (rc != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(rc));
            return 1;
        }
    }

    printf("[main after joins] values:");
    for (int i = 0; i < THREAD_COUNT; i++) {
        printf(" %d", shared_values[i]);
    }
    printf("\n");

    /*
     * Every worker has been joined, so main is the only thread left when it forks.
     * The child gets its own copy of the array.
     */
    child = fork();
    if (child < 0) {
        perror("fork");
        return 1;
    }
    if (child == 0) {
        shared_values[0] += 1000; /* changes the child's copy only */
        printf("[child] PID=%ld changed its copy: first value=%d\n",
               (long)getpid(), shared_values[0]);
        _exit(0);
    }

    /* Parent: wait for the child, and check that it exited normally with status 0. */
    if (waitpid(child, &status, 0) < 0) {
        perror("waitpid");
        return 1;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "child did not exit successfully\n");
        return 1;
    }

    printf("[parent after child] PID=%ld first value is still %d\n",
           (long)getpid(), shared_values[0]);

    return 0;
}
