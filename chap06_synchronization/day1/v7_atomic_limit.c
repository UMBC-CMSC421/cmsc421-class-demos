// v7_atomic_limit.c: atomic variables can't protect a check-then-act.
// DAY 1 SLIDE 47. Now the counter means "items available," and there's 1 item.
// Two consumers each run: if (items > 0) take it.
// Each step is atomic, but the check and the take are two steps. Both consumers
// can see 1, both take it, and items ends up at -1.
// The barriers (start, done) just line up each round. They aren't v4's fence,
// and they don't protect anything.
// EXPECT: the last line is usually a small number above 0, different every run.
// The fix is a mutex around the whole check-and-take, not just the take (Day 2).
// BUILD: gcc -O2 -pthread v7_atomic_limit.c -o v7_atomic_limit
// RUN:   ./v7_atomic_limit
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#define ROUNDS 200000
atomic_int items;                     // items in the buffer
atomic_int taken;                     // items taken this round
pthread_barrier_t start, done;        // main + two consumers meet each round

void *consumer(void *arg) {
    for (int r = 0; r < ROUNDS; r++) {
        pthread_barrier_wait(&start);           // both consumers start together
        if (atomic_load(&items) > 0) {          // check: is there an item?
            atomic_fetch_sub(&items, 1);        // act: take it
            atomic_fetch_add(&taken, 1);
        }
        pthread_barrier_wait(&done);           // finish before main resets items
    }
    return NULL;
}

int main(void) {
    pthread_t c1, c2;
    long both = 0;                              // rounds where both took the one item
    pthread_barrier_init(&start, NULL, 3);      // 2 consumers + main
    pthread_barrier_init(&done, NULL, 3);
    pthread_create(&c1, NULL, consumer, NULL);
    pthread_create(&c2, NULL, consumer, NULL);
    for (int r = 0; r < ROUNDS; r++) {
        atomic_store(&items, 1);                // the producer adds one item
        atomic_store(&taken, 0);
        pthread_barrier_wait(&start);
        pthread_barrier_wait(&done);
        if (atomic_load(&taken) == 2) both++;   // items ended at -1
    }
    pthread_join(c1, NULL);
    pthread_join(c2, NULL);
    printf("rounds: %d, one item each round\n", ROUNDS);
    printf("rounds where both consumers took the one item: %ld\n", both);
    return 0;
}
