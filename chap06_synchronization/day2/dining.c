// dining.c: five philosophers, five chopsticks, a semaphore for each chopstick.
// Slides: Dining Philosophers with Semaphores (Figure 7.1.5); Dining Philosophers in Code.
//   left:       everyone grabs their left chopstick first, and they all get stuck
//   asymmetric: odd seats grab left first, even seats right first, so nobody gets stuck
// A short pause between the two grabs makes sure everyone gets a left one first.
// EXPECT: left: five "has chopstick" lines, then nothing. asymmetric: ends with
// "everyone ate 3 meals".
// BUILD: gcc -O2 -Wall -pthread dining.c -o dining
// RUN:   ./dining left        (it hangs: press Ctrl-C)
//        ./dining asymmetric
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define N 5
#define MEALS 3
sem_t chopstick[N];                       // each starts at 1: one chopstick
int asymmetric;

void *philosopher(void *arg) {
    int i = (int)(long)arg;
    int left = i, right = (i + 1) % N;
    int first = left, second = right;
    if (asymmetric && i % 2 == 0) {       // even seats: right first
        first = right; second = left;
    }
    for (int meal = 1; meal <= MEALS; meal++) {
        sem_wait(&chopstick[first]);
        printf("philosopher %d: has chopstick %d\n", i, first);
        usleep(100000);                   // everyone gets a first one
        sem_wait(&chopstick[second]);
        printf("philosopher %d: eats (meal %d)\n", i, meal);
        usleep(50000);                    // eat
        sem_post(&chopstick[second]);
        sem_post(&chopstick[first]);
        usleep(50000);                    // think
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 2 || (strcmp(argv[1], "left") && strcmp(argv[1], "asymmetric"))) {
        fprintf(stderr, "usage: %s left|asymmetric\n", argv[0]); return 1;
    }
    asymmetric = strcmp(argv[1], "asymmetric") == 0;
    setvbuf(stdout, NULL, _IOLBF, 0);
    for (int i = 0; i < N; i++) sem_init(&chopstick[i], 0, 1);
    pthread_t t[N];
    for (long i = 0; i < N; i++) pthread_create(&t[i], NULL, philosopher, (void *)i);
    for (int i = 0; i < N; i++) pthread_join(t[i], NULL);
    printf("everyone ate %d meals\n", MEALS);
    return 0;
}
