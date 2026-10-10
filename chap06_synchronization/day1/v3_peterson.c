// v3_peterson.c: Peterson's solution (textbook Figure 6.3.1).
// DAY 1 SLIDES 29-33. flag says "I want in," and turn breaks the tie.
// On paper it's correct. On a real CPU, reads and writes can get reordered
// (slide 33), so both threads can end up inside at once.
// EXPECT: some runs come up short of 2,000,000, but not all of them. On an
// emulated VM you might never see it fail. The bug's still there.
// BUILD: gcc -O2 -pthread v3_peterson.c -o v3_peterson
// RUN:   for i in 1 2 3 4 5 6 7 8 9 10; do ./v3_peterson; done
#include <pthread.h>
#include <stdio.h>

#define N 1000000
volatile long counter = 0;            // volatile: every access really goes to memory; not a lock (see v1_race.c)
volatile int flag[2] = {0, 0};        // flag[i] = 1: thread i wants in
volatile int turn = 0;                // who yields when both want in

void *worker(void *arg) {
    int i = *(int *)arg, j = 1 - i;   // read my ID through arg; j is the other ID
    for (int k = 0; k < N; k++) {
        flag[i] = 1;                  // I want in
        turn = j;                     // you go first
        while (flag[j] && turn == j)  // wait while you want in and it is your turn
            ;
        counter++;                    // critical section
        flag[i] = 0;                  // I am done
    }
    return NULL;
}

int main(void) {
    int id0 = 0, id1 = 1;
    pthread_t a, b;
    pthread_create(&a, NULL, worker, &id0);
    pthread_create(&b, NULL, worker, &id1);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("counter = %ld (expected %d)\n", counter, 2 * N);
    return 0;
}
