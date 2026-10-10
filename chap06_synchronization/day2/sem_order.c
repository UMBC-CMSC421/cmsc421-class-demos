// sem_order.c: use a semaphore to make S1 happen before S2.
// Slides: Three Ways to Use a Semaphore; Ordering with a Semaphore in Code.
// P2 starts first and P1 is slow, so on their own, S2 would go first.
//   nosem: no semaphore
//   sem:   synch starts at 0. P2 waits on it, and P1 signals it after S1.
// EXPECT: nosem prints P2 first. sem always prints P1 first, about a second in.
// BUILD: gcc -O2 -Wall -pthread sem_order.c -o sem_order
// RUN:   ./sem_order nosem      ./sem_order sem
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

sem_t synch;
int use_sem;

void *p1(void *arg) {
    sleep(1);                                  // P1 is slow to get going
    printf("P1: S1 (open the log file)\n");
    if (use_sem) sem_post(&synch);             // signal(synch): S1 is done
    return NULL;
}

void *p2(void *arg) {
    if (use_sem) sem_wait(&synch);             // wait(synch): sleep until S1 is done
    printf("P2: S2 (write to the log file)\n");
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 2 || (strcmp(argv[1], "sem") && strcmp(argv[1], "nosem"))) {
        fprintf(stderr, "usage: %s sem|nosem\n", argv[0]); return 1;
    }
    use_sem = strcmp(argv[1], "sem") == 0;
    setvbuf(stdout, NULL, _IOLBF, 0);
    sem_init(&synch, 0, 0);                    // initial value 0: no permit yet
    pthread_t a, b;
    pthread_create(&b, NULL, p2, NULL);        // P2 starts first
    pthread_create(&a, NULL, p1, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    sem_destroy(&synch);
    return 0;
}
