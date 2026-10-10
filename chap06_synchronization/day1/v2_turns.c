// v2_turns.c: strict alternation. You can go in only when it's your turn.
// DAY 1 SLIDES 25-28. Compare the entry and exit lines with v1.
// With equal work, it gets the right answer. With ./v2_turns 3 1, thread 1 takes
// its one turn and leaves, so thread 0 waits forever for a turn nobody will give
// back. That's a progress failure.
// EXPECT: 2,000,000 for equal work. For 3 1 it hangs, and timeout prints exit 124.
// BUILD: gcc -O2 -pthread v2_turns.c -o v2_turns
// RUN:   ./v2_turns 1000000 1000000   (equal work)
//        timeout 5 ./v2_turns 3 1     (thread 0 does 3, thread 1 does 1; stops after 5 s)
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

volatile long counter = 0;            // volatile: every access really goes to memory; not a lock (see v1_race.c)
volatile int turn = 0;                // whose turn: 0 or 1
long n[2];                            // increments for thread 0 and thread 1

void *worker(void *arg) {
    int i = *(int *)arg, j = 1 - i;   // read my ID through arg; j is the other ID
    for (long k = 0; k < n[i]; k++) {
        while (turn != i)             // entry: wait for my turn
            ;
        counter++;                    // critical section
        turn = j;                     // exit: give the turn away
    }
    return NULL;
}

int main(int argc, char **argv) {
    n[0] = argc > 1 ? atol(argv[1]) : 1000000;
    n[1] = argc > 2 ? atol(argv[2]) : 1000000;
    int id0 = 0, id1 = 1;
    pthread_t a, b;
    pthread_create(&a, NULL, worker, &id0);
    pthread_create(&b, NULL, worker, &id1);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("counter = %ld (expected %ld)\n", counter, n[0] + n[1]);
    return 0;
}
