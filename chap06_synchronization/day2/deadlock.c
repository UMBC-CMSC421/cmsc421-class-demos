// deadlock.c: two threads, two locks, opposite order. Then the fix.
// Slides: Rules for Using Mutexes; Deadlock; Deadlock in Code.
//   opposite: thread 1 locks a then b, thread 2 locks b then a, and they get stuck
//   ordered:  both lock a then b, and both finish
// The sleep(1) between the two locks makes sure it gets stuck every time.
// EXPECT: opposite: two "holds ..., wants ..." lines, then nothing.
// ordered: "both threads finished".
// BUILD: gcc -O2 -Wall -pthread deadlock.c -o deadlock
// RUN:   ./deadlock opposite     (it hangs: press Ctrl-C)
//        ./deadlock ordered
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

pthread_mutex_t a = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t b = PTHREAD_MUTEX_INITIALIZER;
int ordered;

const char *name(pthread_mutex_t *l) { return l == &a ? "a" : "b"; }

void lock_both(const char *me, pthread_mutex_t *first,
               pthread_mutex_t *second) {
    pthread_mutex_lock(first);
    printf("%s: holds %s, wants %s\n", me, name(first), name(second));
    sleep(1);                             // the other thread runs now
    pthread_mutex_lock(second);
    printf("%s: holds a and b\n", me);
    pthread_mutex_unlock(second);
    pthread_mutex_unlock(first);
}

void *thread1(void *arg) {
    lock_both("thread 1", &a, &b);
    return NULL;
}

void *thread2(void *arg) {
    if (ordered) lock_both("thread 2", &a, &b);   // same order
    else         lock_both("thread 2", &b, &a);   // opposite order
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 2 || (strcmp(argv[1], "opposite") && strcmp(argv[1], "ordered"))) {
        fprintf(stderr, "usage: %s opposite|ordered\n", argv[0]); return 1;
    }
    ordered = strcmp(argv[1], "ordered") == 0;
    setvbuf(stdout, NULL, _IOLBF, 0);
    pthread_t t1, t2;
    pthread_create(&t1, NULL, thread1, NULL);
    pthread_create(&t2, NULL, thread2, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("both threads finished\n");
    return 0;
}
