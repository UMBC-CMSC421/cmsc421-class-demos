// demo1_race_counter.c
// Demonstrates a data race on counter++ with pthreads.
//
// Observe: final counter is often < expected due to lost updates.
// BUILD: gcc -O2 -pthread demo1_race_counter.c -o demo1
// Run: ./demo1 2 50000000
// Run: ./demo1 8 20000000

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static volatile int64_t counter = 0;   // volatile does NOT make it atomic

typedef struct {
    int64_t iters;
} args_t;

static void* worker(void* arg) {
    args_t* a = (args_t*)arg;
    for (int64_t i = 0; i < a->iters; i++) {
        counter++; // data race: read-modify-write with no synchronization
    }
    return NULL;
}

int main(int argc, char** argv) {
    int threads = 2;
    int64_t iters = 50000000; // 50M per thread by default

    if (argc >= 2) threads = atoi(argv[1]);
    if (argc >= 3) iters = atoll(argv[2]);
    if (threads <= 0 || iters <= 0) {
        fprintf(stderr, "Usage: %s [threads>=1] [iters>=1]\n", argv[0]);
        return 1;
    }

    pthread_t* tids = calloc((size_t)threads, sizeof(pthread_t));
    if (!tids) { perror("calloc"); return 1; }

    args_t a = {.iters = iters};

    for (int i = 0; i < threads; i++) {
        if (pthread_create(&tids[i], NULL, worker, &a) != 0) {
            perror("pthread_create");
            return 1;
        }
    }
    for (int i = 0; i < threads; i++) {
        pthread_join(tids[i], NULL);
    }

    int64_t expected = (int64_t)threads * iters;
    printf("Expected: %lld\n", (long long)expected);
    printf("Actual:   %lld\n", (long long)counter);

    free(tids);
    return 0;
}