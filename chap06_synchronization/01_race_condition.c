// 01_race_condition.c: threads add 1 to a shared counter with no lock, and updates get lost.
// This is the problem all of Chapter 6 is about. After this, go to day1/, then day2/.
// counter++ is really three steps: load, add, store. Two threads can load the
// same value, and then one of the adds gets lost.
// counter is volatile so gcc -O2 doesn't squash the loop into one big add and
// hide the race. It doesn't make anything atomic.
// EXPECT: Actual comes out way below Expected, and different every run.
// BUILD: gcc -O2 -Wall -pthread 01_race_condition.c -o race
// RUN:   ./race                 2 threads, 50,000,000 adds each
//        ./race 8 20000000      8 threads, 20,000,000 each

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static volatile int64_t counter = 0;   // shared by every thread, no lock

typedef struct {
    int64_t iters;
} args_t;

static void* worker(void* arg) {
    const args_t* a = (const args_t*)arg;
    for (int64_t i = 0; i < a->iters; i++) {
        counter++; // data race
    }
    return NULL;
}

int main(int argc, char** argv) {
    int threads = 2;
    int64_t iters = 50000000;

    if (argc >= 2) threads = atoi(argv[1]);
    if (argc >= 3) iters = atoll(argv[2]);

    if (threads <= 0 || iters <= 0) {
        fprintf(stderr, "Usage: %s [threads>=1] [iters>=1]\n", argv[0]);
        return 1;
    }

    pthread_t* tids = calloc((size_t)threads, sizeof(pthread_t));
    if (!tids) {
        perror("calloc");
        return 1;
    }

    args_t a = { .iters = iters };

    for (int i = 0; i < threads; i++) {
        if (pthread_create(&tids[i], NULL, worker, &a) != 0) {
            perror("pthread_create");
            free(tids);
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
