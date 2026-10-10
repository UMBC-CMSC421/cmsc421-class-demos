// v8_mutex.c: Day 1's counter, fixed with a mutex.
// Slides: A Mutex Lock: What It Does; Version 8: The Counter with a Mutex;
//         How Linux Builds pthread_mutex_lock.
// It's v1 plus three lines: the mutex, lock, and unlock. counter doesn't need
// volatile anymore, since the mutex keeps every access in order.
// EXPECT: 2,000,000 every run (or threads x 1,000,000 if you pass a count).
// BUILD: gcc -O2 -Wall -pthread v8_mutex.c -o v8_mutex
// RUN:   ./v8_mutex            2 threads
//        ./v8_mutex 4          4 threads
// Want to see the kernel? Count the futex calls:
//        strace -f -c -e trace=futex ./v8_mutex 1
//        strace -f -c -e trace=futex ./v8_mutex 2
// With 1 thread you get 1 call, and that's pthread_join. A free lock never asks
// the kernel. With 2 threads you get hundreds or thousands: threads sleeping on the lock.
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define N 1000000                         // increments per thread
long counter = 0;                         // shared by all threads
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;   // NEW: one lock for counter

void *worker(void *arg) {
    for (int k = 0; k < N; k++) {
        pthread_mutex_lock(&m);           // NEW: entry section
        counter++;                        // critical section
        pthread_mutex_unlock(&m);         // NEW: exit section
    }
    return NULL;
}

int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 2;
    if (n < 1 || n > 64) { fprintf(stderr, "usage: %s [threads 1..64]\n", argv[0]); return 1; }
    pthread_t t[64];
    for (int i = 0; i < n; i++) pthread_create(&t[i], NULL, worker, NULL);
    for (int i = 0; i < n; i++) pthread_join(t[i], NULL);
    printf("counter = %ld (expected %ld)\n", counter, (long)n * N);
    return 0;
}
