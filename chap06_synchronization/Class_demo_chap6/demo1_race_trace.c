// Trace version of a race condition demo.
// Purpose:
//   Show that incrementing a shared counter without synchronization
//   is not atomic. We break counter++ into:
//      1) read
//      2) add 1 locally
//      3) write back
//
// With two threads, both may read the same old value and both write
// back the same new value, causing a lost update.
//
// BUILD: gcc -O0 -pthread -Wall -Wextra demo1_race_trace.c -o demo1_race_trace
// RUN:   ./demo1_race_trace 2 5

#define _XOPEN_SOURCE 700
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

static int64_t counter = 0;

typedef struct {
    int id;
    int64_t iters;
} args_t;

static void* worker(void* arg) {
    const args_t* a = (const args_t*)arg;

    for (int64_t i = 0; i < a->iters; i++) {
        printf("[T%d] Iteration %lld starting\n", a->id, (long long)i);

        int64_t temp = counter;
        printf("[T%d]   READ  counter = %lld into temp\n",
               a->id, (long long)temp);

        sched_yield();

        temp = temp + 1;
        printf("[T%d]   ADD   temp = temp + 1 => %lld\n",
               a->id, (long long)temp);

        sched_yield();

        counter = temp;
        printf("[T%d]   WRITE counter = %lld\n",
               a->id, (long long)counter);

        printf("[T%d] Iteration %lld done\n\n", a->id, (long long)i);

        usleep(10000); // slow it down so interleaving is easier to observe
    }

    return NULL;
}

int main(int argc, char** argv) {
    int threads = 2;
    int64_t iters = 5;

    if (argc >= 2) threads = atoi(argv[1]);
    if (argc >= 3) iters = atoll(argv[2]);

    if (threads <= 0 || iters <= 0) {
        fprintf(stderr, "Usage: %s [threads>=1] [iters>=1]\n", argv[0]);
        return 1;
    }

    pthread_t* tids = calloc((size_t)threads, sizeof(pthread_t));
    args_t* args = calloc((size_t)threads, sizeof(args_t));
    if (!tids || !args) {
        perror("calloc");
        free(tids);
        free(args);
        return 1;
    }

    printf("=== Race Trace Demo ===\n");
    printf("This program has NO synchronization.\n");
    printf("Watch for two threads reading the same old value before either writes.\n\n");

    for (int i = 0; i < threads; i++) {
        args[i].id = i;
        args[i].iters = iters;
        if (pthread_create(&tids[i], NULL, worker, &args[i]) != 0) {
            perror("pthread_create");
            free(tids);
            free(args);
            return 1;
        }
    }

    for (int i = 0; i < threads; i++) {
        pthread_join(tids[i], NULL);
    }

    int64_t expected = (int64_t)threads * iters;
    printf("Expected final counter = %lld\n", (long long)expected);
    printf("Actual final counter   = %lld\n", (long long)counter);

    if (counter != expected) {
        printf("Result: LOST UPDATE occurred.\n");
    } else {
        printf("Result: This run happened to finish correctly, but the code is still wrong.\n");
    }

    free(tids);
    free(args);
    return 0;
}
