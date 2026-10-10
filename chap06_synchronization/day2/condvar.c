// condvar.c: waiting on a condition, and why you check it in a while loop.
// Slides: A Condition Variable: What It Does; Signal and Wait, or Signal and
//         Continue?; Pthreads Condition Variables; Condition Variables in Code.
// Two consumers wait for items. The producer adds one item and wakes everybody
// up (broadcast), so one consumer always wakes up to find nothing there.
// A spurious wakeup does the same thing.
//   while: each consumer checks again after it wakes up (correct)
//   if:    it checks only once (the bug)
// EXPECT: while: one consumer takes the item, the other goes back to sleep, and
// the last line says "no". if: both take it, items hits -1, and the last line
// says "YES (bug)".
// BUILD: gcc -O2 -Wall -pthread condvar.c -o condvar
// RUN:   ./condvar while      ./condvar if
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  c = PTHREAD_COND_INITIALIZER;
int items = 0;                            // the condition: items > 0
int use_while, went_negative;
struct timespec t0;

void say(const char *who, const char *what) {   // time, who, what, items
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    double s = (t.tv_sec - t0.tv_sec) + (t.tv_nsec - t0.tv_nsec) / 1e9;
    printf("%3.1f s  %s: %s (items %d)\n", s, who, what, items);
}

void wait_for_item(const char *me) {      // called holding m
    say(me, "wait: unlock m, sleep");
    pthread_cond_wait(&c, &m);            // returns holding m again
    say(me, "woke up holding m");
}

void *consumer(void *arg) {
    pthread_mutex_lock(&m);
    say(arg, "lock m");
    if (use_while)
        while (items == 0) wait_for_item(arg);   // recheck every time
    else
        if (items == 0) wait_for_item(arg);      // BUG: check once
    items--;                              // take an item
    if (items < 0) went_negative = 1;
    say(arg, "took an item");
    pthread_mutex_unlock(&m);
    return NULL;
}

void *producer(void *arg) {
    for (int i = 1; i <= 2; i++) {
        sleep(1);                         // let both consumers fall asleep
        pthread_mutex_lock(&m);
        items++;
        say("producer", "add 1 item; broadcast");
        pthread_cond_broadcast(&c);       // wake every waiter
        say("producer", "unlock m");      // signal and continue: still holds m
        pthread_mutex_unlock(&m);
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 2 || (strcmp(argv[1], "while") && strcmp(argv[1], "if"))) {
        fprintf(stderr, "usage: %s while|if\n", argv[0]); return 1;
    }
    use_while = strcmp(argv[1], "while") == 0;
    setvbuf(stdout, NULL, _IOLBF, 0);     // print each line as it happens
    clock_gettime(CLOCK_MONOTONIC, &t0);
    pthread_t a, b, p;
    pthread_create(&a, NULL, consumer, "consumer A");
    pthread_create(&b, NULL, consumer, "consumer B");
    pthread_create(&p, NULL, producer, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    pthread_join(p, NULL);
    printf("items ever below 0: %s\n", went_negative ? "YES (bug)" : "no");
    return 0;
}
