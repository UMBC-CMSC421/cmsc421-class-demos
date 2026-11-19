// errorcheck.c: one thread, one mutex, locked twice. What does Pthreads do?
// Ch 8 Day 1 Enrichment slide: Relocking Your Own Mutex.
//   PTHREAD_MUTEX_ERRORCHECK: the second lock returns EDEADLK right away.
//   default mutex: the second lock just waits, forever. We use pthread_mutex_timedlock()
//   with a 2-second limit so the demo can report that wait instead of hanging.
// POSIX leaves the default type's relock undefined; this shows what glibc does on the VM.
// EXPECT: errorcheck: "second lock: EDEADLK". default: "second lock: ETIMEDOUT after 2 s".
// BUILD: make            (or: gcc -O2 -Wall -Wextra -pthread errorcheck.c -o errorcheck)
// RUN:   ./errorcheck
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *name(int err) {
    return err == 0 ? "0 (success)" : err == EDEADLK ? "EDEADLK" : err == ETIMEDOUT ? "ETIMEDOUT" : strerror(err);
}

int main(void) {
    pthread_mutexattr_t attr;
    pthread_mutex_t m;

    // 1. An error-checking mutex remembers its owner.
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&m, &attr);
    printf("errorcheck mutex, first lock:  %s\n", name(pthread_mutex_lock(&m)));
    printf("errorcheck mutex, second lock: %s\n", name(pthread_mutex_lock(&m)));
    pthread_mutex_unlock(&m);
    pthread_mutex_destroy(&m);

    // 2. A default mutex does not check: the owner waits on itself.
    pthread_mutex_init(&m, NULL);
    printf("default mutex, first lock:     %s\n", name(pthread_mutex_lock(&m)));
    struct timespec limit;
    clock_gettime(CLOCK_REALTIME, &limit);
    limit.tv_sec += 2;                                   // give up after 2 seconds
    printf("default mutex, second lock:    %s after 2 s\n", name(pthread_mutex_timedlock(&m, &limit)));
    pthread_mutex_unlock(&m);
    return 0;
}
