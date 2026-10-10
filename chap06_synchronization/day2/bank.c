// bank.c: Activity 6.5.2. Where do acquire() and release() go in withdraw()?
// Slides: Your Turn: Lock the Account; The Bank in Code.
// Every round the balance is 100, and two customers try to take out 80 at the
// same time. Only one should get paid.
//   none:   no lock
//   inside: lock just the subtraction (the popular wrong answer)
//   whole:  lock the check and the subtraction together (the right answer)
// work() is a short delay between the check and the subtraction, so the race shows up more.
// EXPECT: none and inside: both paid in hundreds to a few thousand of the
// 100,000 rounds. whole: 0 and 0.
// BUILD: gcc -O2 -Wall -pthread bank.c -o bank
// RUN:   ./bank none    ./bank inside    ./bank whole
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define ROUNDS 100000
enum { NONE, INSIDE, WHOLE } mode;
volatile long balance;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t start, done;
int paid[2];                                        // per customer, this round

static void work(void) { for (volatile int i = 0; i < 200; i++) ; }

int withdraw(long amount) {                         // returns 1 if the cash was handed out
    int paid = 0;
    if (mode == WHOLE) pthread_mutex_lock(&m);      // acquire() before the if
    if (amount <= balance) {                        // check
        work();
        if (mode == INSIDE) pthread_mutex_lock(&m); // too late: the check is outside
        balance -= amount;                          // act
        paid = 1;
        if (mode == INSIDE) pthread_mutex_unlock(&m);
    }
    if (mode == WHOLE) pthread_mutex_unlock(&m);    // release() after the if
    return paid;
}

void *customer(void *arg) {
    int me = (int)(long)arg;
    for (int r = 0; r < ROUNDS; r++) {
        pthread_barrier_wait(&start);               // both withdraw together
        paid[me] = withdraw(80);
        pthread_barrier_wait(&done);
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s none|inside|whole\n", argv[0]); return 1; }
    if      (!strcmp(argv[1], "none"))   mode = NONE;
    else if (!strcmp(argv[1], "inside")) mode = INSIDE;
    else if (!strcmp(argv[1], "whole"))  mode = WHOLE;
    else { fprintf(stderr, "usage: %s none|inside|whole\n", argv[0]); return 1; }
    pthread_t a, b;
    long twice = 0, negative = 0;
    pthread_barrier_init(&start, NULL, 3);          // 2 customers + main
    pthread_barrier_init(&done, NULL, 3);
    pthread_create(&a, NULL, customer, (void *)0L);
    pthread_create(&b, NULL, customer, (void *)1L);
    for (int r = 0; r < ROUNDS; r++) {
        balance = 100;
        pthread_barrier_wait(&start);
        pthread_barrier_wait(&done);
        if (paid[0] && paid[1]) twice++;            // the bank paid out 160 from 100
        if (balance < 0) negative++;                // 100 - 80 - 80 = -60
    }
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("%-6s both paid: %ld  below 0: %ld\n", argv[1], twice, negative);
    return 0;
}
