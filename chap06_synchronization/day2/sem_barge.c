// sem_barge.c: when signal() wakes someone up, do they actually get the permit?
// Slides: Does a Semaphore Meet the Requirements?; How Linux Builds sem_wait and sem_post.
// The semaphore starts at 1, so it works like a lock. A busy thread grabs it,
// gives it back, and grabs it again, nonstop. Main asks for it 200 times, and
// we count how many times the busy thread cuts in line while main is waiting.
//   posix     glibc's sem_t: sem_post wakes main, but whoever asks first gets the permit
//   textbook  the textbook's semaphore (Sec. 6.6): signal() hands the permit
//             straight to the first thread in line
// EXPECT: posix: the busy thread cuts in several times per wait on average, and
// tens to thousands of times at worst. textbook: average 0.0, most 1.
// Works best on 2 or more CPUs. The numbers jump around a lot between machines.
// BUILD: gcc -O2 -Wall -pthread sem_barge.c -o sem_barge
// RUN:   ./sem_barge posix      ./sem_barge textbook
#include <linux/futex.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#define ROUNDS 200
int textbook;
atomic_long busy_entries;                 // every time the busy thread got in
atomic_long got_in_while_asking;          // only the times main was waiting
atomic_int asking, stop;

// ---- The textbook semaphore (Sec. 6.6), with a FIFO list of sleepers ----
// As in the textbook, wait() and signal() are short critical sections, guarded
// here by a ticket spinlock, which serves threads in the order they arrive.
// sleep() and wakeup() are system calls: on Linux, the futex system call.
struct waiter { atomic_int woken; struct waiter *next; };
struct tsem {
    atomic_uint next_ticket, serving;     // the ticket spinlock
    int value;                            // negative: number of sleepers
    struct waiter *head, *tail;           // FIFO list
} ts = { 0, 0, 1, NULL, NULL };

static void guard(struct tsem *s) {
    unsigned my = atomic_fetch_add(&s->next_ticket, 1);
    while (atomic_load(&s->serving) != my)
        ;                                 // spin while another thread is inside
                                          // (short, unless that thread is preempted)
}
static void unguard(struct tsem *s) { atomic_fetch_add(&s->serving, 1); }

static void sleep_until_woken(atomic_int *w) {    // the textbook's sleep()
    while (!atomic_load(w))
        syscall(SYS_futex, w, FUTEX_WAIT_PRIVATE, 0, NULL, NULL, 0);
}
// The woken thread may return before FUTEX_WAKE runs; a stray wake later is
// harmless, because sleep_until_woken checks the flag in a loop.
static void wakeup(atomic_int *w) {               // the textbook's wakeup(P)
    atomic_store(w, 1);
    syscall(SYS_futex, w, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
}

void t_wait(struct tsem *s) {
    guard(s);
    s->value--;
    if (s->value < 0) {                   // no permit: join the end of the list
        struct waiter me = { 0, NULL };
        if (s->tail) s->tail->next = &me; else s->head = &me;
        s->tail = &me;
        unguard(s);
        sleep_until_woken(&me.woken);     // signal() picks me and hands me the permit
        return;
    }
    unguard(s);
}

void t_signal(struct tsem *s) {
    guard(s);
    s->value++;
    if (s->value <= 0) {                  // someone is asleep: hand the permit
        struct waiter *w = s->head;       // to the longest waiter
        s->head = w->next;
        if (!s->head) s->tail = NULL;
        unguard(s);
        wakeup(&w->woken);
        return;
    }
    unguard(s);
}
// ---- end of the textbook semaphore ----

sem_t ps;                                 // the POSIX semaphore
void take(void) { if (textbook) t_wait(&ts);   else sem_wait(&ps); }
void give(void) { if (textbook) t_signal(&ts); else sem_post(&ps); }

static double now(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}
static void work(int n) { volatile int x = 0; for (int i = 0; i < n; i++) x++; }

void *busy(void *arg) {
    (void)arg;
    while (!atomic_load(&stop)) {
        take();
        atomic_fetch_add(&busy_entries, 1);
        if (atomic_load(&asking)) atomic_fetch_add(&got_in_while_asking, 1);
        work(2000);                       // a short critical section
        give();
        work(200);                        // then asks again almost at once
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 2 || (strcmp(argv[1], "posix") && strcmp(argv[1], "textbook"))) {
        fprintf(stderr, "usage: %s posix|textbook\n", argv[0]); return 1;
    }
    textbook = strcmp(argv[1], "textbook") == 0;
    sem_init(&ps, 0, 1);                  // starts at 1: a lock

    pthread_t b;
    double start = now();
    pthread_create(&b, NULL, busy, NULL);
    usleep(10000);                        // let the busy thread get going

    long most = 0, total = 0;
    double longest = 0;
    for (int r = 0; r < ROUNDS; r++) {
        atomic_store(&got_in_while_asking, 0);
        double asked = now();
        atomic_store(&asking, 1);
        take();                           // main asks for the permit
        atomic_store(&asking, 0);
        double waited = now() - asked;
        long passed = atomic_load(&got_in_while_asking);
        give();
        total += passed;
        if (passed > most) most = passed;
        if (waited > longest) longest = waited;
        usleep(1000);                     // main asks about 1,000 times a second
    }
    atomic_store(&stop, 1);
    pthread_join(b, NULL);
    double ms = (now() - start) * 1e3;
    printf("%-8s busy cut ahead: average %4.1f, most %3ld | main's longest wait %4.0f us"
           " | busy entries per ms %4.0f\n",
           argv[1], (double)total / ROUNDS, most, longest * 1e6,
           atomic_load(&busy_entries) / ms);
    sem_destroy(&ps);
    return 0;
}
