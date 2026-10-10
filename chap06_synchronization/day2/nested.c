// nested.c: one update that needs two locks.
// Slides: Version 8: The Counter with a Mutex; Nesting Two Locks in Code.
// Two accounts with $100 each. Thread 1 moves $1 from a to b a million times,
// thread 2 moves it back, and a checker keeps making sure a + b is still 200.
// Everybody locks a first, then b, even thread 2. That's why transfer() locks
// a.m and b.m by name, not from and to. Same order for everyone means no
// deadlock (see deadlock.c). Unlock in reverse: b, then a.
// EXPECT: the checker finds a + b != 200 in 0 checks, and it ends at a = 100, b = 100.
// BUILD: gcc -O2 -Wall -pthread nested.c -o nested
// RUN:   ./nested
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#define N 1000000                         // transfers per thread

typedef struct { long balance; pthread_mutex_t m; } account_t;
account_t a = { 100, PTHREAD_MUTEX_INITIALIZER };
account_t b = { 100, PTHREAD_MUTEX_INITIALIZER };
atomic_int done = 0;                      // set by main when the transfers finish

void transfer(account_t *from, account_t *to, long amt) {
    pthread_mutex_lock(&a.m);             // 1. outer lock: always a first
    pthread_mutex_lock(&b.m);             // 2. inner lock: then b
    from->balance -= amt;                 //    both accounts are ours now
    to->balance   += amt;
    pthread_mutex_unlock(&b.m);           // 3. release the inner lock first
    pthread_mutex_unlock(&a.m);           // 4. release the outer lock last
}

void *a_to_b(void *arg) {                 // thread 1: money moves a -> b
    for (int i = 0; i < N; i++) transfer(&a, &b, 1);
    return NULL;
}

void *b_to_a(void *arg) {                 // thread 2: money moves b -> a,
    for (int i = 0; i < N; i++) transfer(&b, &a, 1);   // but it still locks a first
    return NULL;
}

void *checker(void *arg) {
    long checks = 0, bad = 0;
    while (!done) {
        pthread_mutex_lock(&a.m);         // same order: a, then b
        pthread_mutex_lock(&b.m);
        if (a.balance + b.balance != 200) bad++;
        checks++;
        pthread_mutex_unlock(&b.m);
        pthread_mutex_unlock(&a.m);
    }
    printf("checker: %ld checks, a + b != 200 in %ld of them\n", checks, bad);
    return NULL;
}

int main(void) {
    pthread_t t1, t2, c;
    pthread_create(&c, NULL, checker, NULL);
    pthread_create(&t1, NULL, a_to_b, NULL);
    pthread_create(&t2, NULL, b_to_a, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    done = 1;
    pthread_join(c, NULL);
    printf("a = %ld, b = %ld, a + b = %ld (expected 200)\n",
           a.balance, b.balance, a.balance + b.balance);
    return 0;
}
