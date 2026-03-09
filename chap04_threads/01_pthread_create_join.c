/*
 * Chapter 4: Threads
 * Demo 01: Create and join a thread
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static void *worker(void *arg)
{
    const char *message = arg;
    printf("[thread] %s\n", message);
    return NULL;
}

int main(void)
{
    pthread_t thread;

    if (pthread_create(&thread, NULL, worker, "Hello from a POSIX thread") != 0) {
        perror("pthread_create");
        return 1;
    }

    if (pthread_join(thread, NULL) != 0) {
        perror("pthread_join");
        return 1;
    }

    printf("[main  ] thread joined successfully\n");
    return 0;
}
