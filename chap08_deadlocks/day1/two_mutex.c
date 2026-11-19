// two_mutex.c: the textbook's two-mutex deadlock (Figure 8.2.1), then the fix.
// Ch 8 Day 1 slides: Two Threads, Two Mutexes; Linux: A Deadlock on the VM; Lock Ordering Fixes It.
//   textbook: thread_one locks first_mutex then second_mutex,
//             thread_two locks second_mutex then first_mutex. They get stuck.
//   ordered:  both threads lock first_mutex, then second_mutex. Both finish.
//   serial:   textbook lock orders, but thread_two starts only after thread_one is done.
//             It never hangs, yet the opposite orders are still there: a tool such as
//             ThreadSanitizer (gcc -fsanitize=thread) reports them as a lock-order inversion.
// The sleep(1) after the first lock makes the bad interleaving happen every run.
// Without it the deadlock still exists; it just shows up only some of the time.
// EXPECT: textbook: main's start line and two "holds ... / wants ..." lines, then nothing (Ctrl-C to stop).
//         ordered and serial: "both threads finished".
// BUILD: make            (or: gcc -O2 -Wall -Wextra -pthread two_mutex.c -o two_mutex)
// RUN:   ./two_mutex textbook      (hangs: inspect it from a second terminal, then Ctrl-C)
//        ./two_mutex ordered
//        make tsan && ./two_mutex_tsan serial
#define _GNU_SOURCE              // for gettid()
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

pthread_mutex_t first_mutex;     // the two locks from the textbook figure
pthread_mutex_t second_mutex;
int ordered;                     // 1: both threads use the same lock order
int serial;                      // 1: run thread_one to the end before starting thread_two

// thread_one runs in this function
void *do_work_one(void *param) {
    (void)param;
    pthread_mutex_lock(&first_mutex);
    printf("thread_one (TID %d): holds first_mutex, wants second_mutex\n", gettid());
    if (!serial) sleep(1);                     // give thread_two time to take its first lock
    pthread_mutex_lock(&second_mutex);         // textbook mode: blocks forever here
    printf("thread_one: holds both, doing work\n");
    pthread_mutex_unlock(&second_mutex);
    pthread_mutex_unlock(&first_mutex);
    return NULL;
}

// thread_two runs in this function
void *do_work_two(void *param) {
    (void)param;
    pthread_mutex_t *a = ordered ? &first_mutex : &second_mutex;   // which lock first
    pthread_mutex_t *b = ordered ? &second_mutex : &first_mutex;   // which lock second
    pthread_mutex_lock(a);
    printf("thread_two (TID %d): holds %s, wants %s\n", gettid(),
           a == &first_mutex ? "first_mutex" : "second_mutex",
           b == &first_mutex ? "first_mutex" : "second_mutex");
    if (!serial) sleep(1);
    pthread_mutex_lock(b);                     // textbook mode: blocks forever here
    printf("thread_two: holds both, doing work\n");
    pthread_mutex_unlock(b);
    pthread_mutex_unlock(a);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 2 || (strcmp(argv[1], "textbook") && strcmp(argv[1], "ordered") && strcmp(argv[1], "serial"))) {
        fprintf(stderr, "usage: %s textbook|ordered|serial\n", argv[0]);
        return 1;
    }
    ordered = strcmp(argv[1], "ordered") == 0;
    serial = strcmp(argv[1], "serial") == 0;
    setvbuf(stdout, NULL, _IOLBF, 0);          // print each line right away, even into a pipe
    pthread_mutex_init(&first_mutex, NULL);    // as in the textbook: default attributes
    pthread_mutex_init(&second_mutex, NULL);
    printf("main (PID %d): starting thread_one and thread_two\n", getpid());
    pthread_t t1, t2;
    pthread_create(&t1, NULL, do_work_one, NULL);
    if (serial) pthread_join(t1, NULL);        // serial: let thread_one finish first
    pthread_create(&t2, NULL, do_work_two, NULL);
    if (!serial) pthread_join(t1, NULL);       // textbook mode: main waits here forever
    pthread_join(t2, NULL);
    printf("both threads finished\n");
    return 0;
}
