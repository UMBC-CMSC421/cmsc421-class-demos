// v4_fence.c: Peterson's again, plus one memory barrier (fence).
// DAY 1 SLIDES 36-39. The only change from v3 is the __sync_synchronize() line.
// The fence makes my writes land before I read your flag. It isn't a lock by itself.
// EXPECT: 2,000,000 every run. Compare with v3.
// It's still an experiment: flag and turn aren't atomic, so don't copy this as a real lock.
// BUILD: gcc -O2 -pthread v4_fence.c -o v4_fence
// RUN:   ./v4_fence
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
        __sync_synchronize();         // full memory barrier before checking peer state
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
