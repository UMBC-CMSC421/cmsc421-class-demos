// v1_race.c: two threads add 1 to the same counter, with no protection.
// DAY 1 SLIDES 9-19. Start here. Every later version fixes this one a different way.
// Both threads can read the same old value, and then one write wipes out the other.
// EXPECT: way below 2,000,000, and a different number every run.
// The race is on purpose. If you ever get 2,000,000, that's luck, not safety.
// BUILD: gcc -O2 -pthread v1_race.c -o v1_race
// RUN:   ./v1_race
#include <pthread.h>
#include <stdio.h>

#define N 1000000                     // increments per thread

// volatile means every read and write of counter really happens in memory.
// Without it, gcc -O2 turns the whole loop into one "add 1,000,000" and you'd
// never see the race. With it, each counter++ stays a load, add, store (slide 12).
// It does NOT make counter++ atomic, and it doesn't keep threads out of each other's way.
volatile long counter = 0;            // shared by both threads

void *worker(void *arg) {
    for (int k = 0; k < N; k++)
        counter++;                    // the critical section
    return NULL;
}

int main(void) {
    pthread_t a, b;                   // handles for the two worker threads
    pthread_create(&a, NULL, worker, NULL); // start worker A; default attributes
    pthread_create(&b, NULL, worker, NULL); // start worker B; no argument needed
    pthread_join(a, NULL);            // wait for A; this is NOT a counter lock
    pthread_join(b, NULL);            // wait for B before reading the result
    printf("counter = %ld (expected %d)\n", counter, 2 * N);
    return 0;
}
