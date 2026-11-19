// livelock.c: the textbook's trylock version (Figure 8.2.2): no thread blocks, nobody gets anywhere.
// Ch 8 Day 1 slides: Livelock: Busy, but No Progress; Linux: Livelock, Then Backoff; Deadlock vs. Livelock.
//   lockstep: both threads retry at the same moment, every round. They livelock.
//   backoff:  after a failed try, each thread waits a random time first. One gets through.
// A barrier forces the unlucky timing (every round in lockstep, round 1 in backoff),
// so the collision shows up on every run.
// The program stops each mode after 8 rounds so it never spins forever.
// EXPECT: lockstep: 8 rounds of "busy, released", "no progress after 8 rounds".
//         backoff:  round 1 collides; after random waits, both threads finish.
// BUILD: make            (or: gcc -O2 -Wall -Wextra -pthread livelock.c -o livelock)
// RUN:   ./livelock lockstep
//        ./livelock backoff
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ROUNDS 8

pthread_mutex_t first_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t second_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t step;          // lockstep mode: both threads pass each point together
int backoff;                     // 1: wait a random time after a failed try

struct who { const char *name; pthread_mutex_t *mine, *other; unsigned seed; };

void *worker(void *arg) {
    struct who *w = arg;
    for (int round = 1; round <= ROUNDS; round++) {
        int together = !backoff || round == 1;           // round 1 always collides, in both modes
        if (together) pthread_barrier_wait(&step);        // start the round at the same moment
        pthread_mutex_lock(w->mine);                     // take my first lock
        if (together) pthread_barrier_wait(&step);        // both now hold one lock
        if (pthread_mutex_trylock(w->other) == 0) {      // 0 means I got the second lock
            printf("%s round %d: got both locks, doing work\n", w->name, round);
            pthread_mutex_unlock(w->other);
            pthread_mutex_unlock(w->mine);
            printf("%s finished\n", w->name);
            return NULL;
        }
        if (together) pthread_barrier_wait(&step);        // both tries have failed
        printf("%s round %d: other lock busy, released mine\n", w->name, round);
        pthread_mutex_unlock(w->mine);                   // give up and try again
        if (backoff) usleep((useconds_t)(rand_r(&w->seed) % 50000));   // wait 0 to 50 ms, then retry
    }
    printf("%s: no progress after %d rounds\n", w->name, ROUNDS);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 2 || (strcmp(argv[1], "lockstep") && strcmp(argv[1], "backoff"))) {
        fprintf(stderr, "usage: %s lockstep|backoff\n", argv[0]);
        return 1;
    }
    backoff = strcmp(argv[1], "backoff") == 0;
    setvbuf(stdout, NULL, _IOLBF, 0);
    pthread_barrier_init(&step, NULL, 2);
    struct who one = {"thread_one", &first_mutex, &second_mutex, 1};
    struct who two = {"thread_two", &second_mutex, &first_mutex, 2};
    pthread_t t1, t2;
    pthread_create(&t1, NULL, worker, &one);
    pthread_create(&t2, NULL, worker, &two);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return 0;
}
