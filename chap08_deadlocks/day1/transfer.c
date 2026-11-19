// transfer.c: lock ordering breaks when the locks are picked at run time (Figure 8.5.1).
// Ch 8 Day 1 slide: The Dynamic Lock-Order Trap.
//   naive:   transaction() locks "from" then "to". One thread moves checking -> savings,
//            the other moves savings -> checking. Opposite order, so they can deadlock.
//   ordered: transaction() always locks the account with the smaller id first.
// The usleep between the two locks makes the bad interleaving likely on every run.
// EXPECT: naive:   it hangs (the course capture script stops it after 5 seconds).
//         ordered: 100000 transfers each way, total still 1000.
// BUILD: make            (or: gcc -O2 -Wall -Wextra -pthread transfer.c -o transfer)
// RUN:   ./transfer naive      (hangs: Ctrl-C)
//        ./transfer ordered
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define TRANSFERS 100000

typedef struct {
    int id;                      // a stable, unique number: the lock order
    int balance;
    pthread_mutex_t lock;        // one lock per account
} Account;

Account checking = {1, 500, PTHREAD_MUTEX_INITIALIZER};
Account savings  = {2, 500, PTHREAD_MUTEX_INITIALIZER};
int ordered;

void transaction(Account *from, Account *to, int amount) {
    Account *first = from, *second = to;                 // naive: the order the caller gave
    if (ordered && to->id < from->id) {                  // ordered: smaller id first, always
        first = to; second = from;
    }
    pthread_mutex_lock(&first->lock);
    usleep(1);                                           // widen the window between the locks
    pthread_mutex_lock(&second->lock);
    from->balance -= amount;                             // withdraw
    to->balance += amount;                               // deposit
    pthread_mutex_unlock(&second->lock);
    pthread_mutex_unlock(&first->lock);
}

void *to_savings(void *arg)  { (void)arg; for (int i = 0; i < TRANSFERS; i++) transaction(&checking, &savings, 25); return NULL; }
void *to_checking(void *arg) { (void)arg; for (int i = 0; i < TRANSFERS; i++) transaction(&savings, &checking, 25); return NULL; }

int main(int argc, char **argv) {
    if (argc < 2 || (strcmp(argv[1], "naive") && strcmp(argv[1], "ordered"))) {
        fprintf(stderr, "usage: %s naive|ordered\n", argv[0]);
        return 1;
    }
    ordered = strcmp(argv[1], "ordered") == 0;
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("%s: %d transfers each way, starting total %d\n", argv[1], TRANSFERS,
           checking.balance + savings.balance);
    pthread_t a, b;
    pthread_create(&a, NULL, to_savings, NULL);
    pthread_create(&b, NULL, to_checking, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("done: checking %d, savings %d, total %d\n", checking.balance, savings.balance,
           checking.balance + savings.balance);
    return 0;
}
