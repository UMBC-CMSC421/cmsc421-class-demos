/*
 * Chapter 4: Threads
 * Demo 01: Create one thread and wait for it
 *
 * What's going on
 *   Every program starts with one thread, the one running main(). pthread_create()
 *   starts a second thread inside the same process, and that thread runs worker().
 *   pthread_join() makes main wait until worker() returns. Without the join, main
 *   could return first, and when main returns the whole process ends, taking the
 *   new thread with it.
 *
 *   pthread functions return 0 when they work and an error number when they don't.
 *   They don't set errno, so this demo passes the return value to strerror() instead
 *   of calling perror().
 *
 * Build
 *   make
 *
 * Run
 *   ./01_pthread_create_join
 *
 * What to look for
 *   The [thread] line always comes before the [main  ] line, because main can't get
 *   past pthread_join() until the thread has printed and returned.
 *
 * Other ways to see it
 *   - Delete the pthread_join() call (and its if block) and run it a few times. Most
 *     runs print only the [main  ] line: the process ended before the thread printed.
 */
#include <pthread.h> /* pthread_t, pthread_create(), pthread_join() */
#include <stdio.h>   /* printf(), fprintf() */
#include <string.h>  /* strerror() */

/*
 * A thread function always takes one void * and returns one void *. A void * is a
 * pointer to anything, so one function shape works for every kind of argument.
 * arg is the last argument main passed to pthread_create().
 */
static void *worker(void *arg)
{
    const char *message = arg; /* we know main passed a string, so treat it as one */
    printf("[thread] %s\n", message);
    return NULL; /* nothing to hand back to main */
}

int main(void)
{
    pthread_t thread; /* filled in by pthread_create(); names the new thread */
    int rc;           /* return code from each pthread call */

    /*
     * Arguments: where to store the thread's ID, attributes (NULL means defaults), the
     * function the thread runs, and the one argument that function receives.
     */
    rc = pthread_create(&thread, NULL, worker, "Hello from a POSIX thread");
    if (rc != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(rc));
        return 1;
    }

    /* Wait here until worker() returns. NULL means we don't want its return value. */
    rc = pthread_join(thread, NULL);
    if (rc != 0) {
        fprintf(stderr, "pthread_join: %s\n", strerror(rc));
        return 1;
    }

    printf("[main  ] thread joined successfully\n");
    return 0;
}
