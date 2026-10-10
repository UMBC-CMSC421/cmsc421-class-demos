// sem_permits.c: a semaphore is a count of permits plus a line of sleepers.
// Slides: A Semaphore: What It Does; A Semaphore in Code.
// It starts with 2 permits. Five threads show up 0.2 s apart, and each one
// keeps a permit for 1 s.
// EXPECT: T1 and T2 get permits right away. T3, T4, and T5 wait. At 1.0 s, T1
// gives its permit back and T3 grabs it. "holding" never goes over the number of permits.
// BUILD: gcc -O2 -Wall -pthread sem_permits.c -o sem_permits
// RUN:   ./sem_permits        2 permits
//        ./sem_permits 1      1 permit, so it works like a lock
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define THREADS 5
sem_t permits;
atomic_int holding;                       // threads holding a permit right now
struct timespec t0;

void say(int id, const char *what) {      // print: time, thread, event
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    double s = (t.tv_sec - t0.tv_sec) + (t.tv_nsec - t0.tv_nsec) / 1e9;
    printf("%4.1f s  T%d: %-14s holding: %d\n", s, id, what, atomic_load(&holding));
}

void *worker(void *arg) {
    int id = (int)(long)arg;
    usleep(200000 * (id - 1));            // T1 first, then T2, ...
    say(id, "wait()");
    sem_wait(&permits);                   // take a permit, or sleep
    atomic_fetch_add(&holding, 1);
    say(id, "  got a permit");
    sleep(1);                             // use the resource
    atomic_fetch_sub(&holding, 1);
    say(id, "signal()");
    sem_post(&permits);                   // return it; wake a sleeper
    return NULL;
}

int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 2;
    if (n < 1 || n > THREADS) n = 2;
    setvbuf(stdout, NULL, _IOLBF, 0);     // print each line as it happens
    sem_init(&permits, 0, n);             // initial value = permits
    clock_gettime(CLOCK_MONOTONIC, &t0);
    printf("semaphore starts at %d\n", n);
    pthread_t t[THREADS];
    for (long i = 0; i < THREADS; i++)
        pthread_create(&t[i], NULL, worker, (void *)(i + 1));
    for (int i = 0; i < THREADS; i++) pthread_join(t[i], NULL);
    sem_destroy(&permits);
    return 0;
}
