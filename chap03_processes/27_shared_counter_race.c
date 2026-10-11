/*
 * Chapter 3: Processes
 * Demo 27: A race condition on shared memory
 *
 * What's going on
 *   Two processes share one long through a MAP_SHARED mapping. Each one adds 1 to it
 *   50 million times, so the answer should be 100,000,000. But counter++ is three
 *   machine steps: load the value into a register, add 1, store it back. If one
 *   process is interrupted between the load and the store, the other's updates in
 *   between are overwritten and lost. That's a race condition, the same one as the
 *   T0 to T5 table on the "Why Shared Memory Needs Synchronization" slide.
 *
 *   --atomic makes each increment one indivisible hardware operation. That fixes this
 *   counter; Chapter 6 covers the general tools (mutexes, semaphores).
 *
 * Build
 *   make
 *
 * Run
 *   ./27_shared_counter_race
 *   ./27_shared_counter_race --atomic
 *
 * What to look for
 *   Without --atomic the total is less than 100,000,000, and it changes from run to
 *   run. With --atomic it's exactly 100,000,000 every time (and slower).
 *   With two or more CPUs the processes collide constantly. With one CPU they only
 *   interleave when the timer interrupts one of them mid-increment, so fewer updates
 *   are lost; that's why the loop is long enough to span many time slices.
 *
 * Other ways to see it
 *   nproc                       how many CPUs the race can use at once
 *   taskset -c 0 ./27_shared_counter_race     force one CPU: usually fewer lost updates
 */
#define _DEFAULT_SOURCE /* MAP_ANONYMOUS */
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* How many times each child adds 1. The L makes it a long number. */
#define ROUNDS 50000000L

int main(int argc, char *argv[])
{
    /* 1 if you passed --atomic, 0 otherwise (strcmp returns 0 when strings match). */
    int use_atomic = (argc == 2 && strcmp(argv[1], "--atomic") == 0);

    /* One long, shared between parent and children because the mapping is MAP_SHARED.
       MAP_ANONYMOUS means it's plain memory with no file or name behind it, so the
       fd is -1. Children made by fork() keep the mapping, and they all see the same
       long. (A regular variable would be copied by fork, so each child would have
       its own.) sizeof(long) is how many bytes a long takes.
       volatile tells the compiler to really read and write memory every time,
       instead of keeping the value in a register where the other process can't see it.
       mmap() returns MAP_FAILED, not NULL, when it fails. */
    volatile long *counter = mmap(NULL, sizeof(long), PROT_READ | PROT_WRITE,
                                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (counter == MAP_FAILED) {
        perror("mmap"); /* perror prints your text plus the reason the call failed */
        return 1;
    }
    *counter = 0;
    fflush(stdout); /* empty stdout's buffer before fork so nothing prints twice */

    /* Start two children. Each one runs the loop below and then exits. */
    for (int c = 0; c < 2; c++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return 1;
        }
        if (pid == 0) {
            /* Child: hammer on the shared counter. */
            for (long i = 0; i < ROUNDS; i++) {
                if (use_atomic) {
                    /* A gcc built-in. The CPU does the read, add, and write as one step
                       that the other process can't cut into. */
                    __atomic_fetch_add(counter, 1, __ATOMIC_SEQ_CST); /* one indivisible step */
                } else {
                    *counter = *counter + 1; /* load, add, store: three steps */
                }
            }
            /* _exit() ends the child now, so it never goes back into the for loop
               and forks children of its own. */
            _exit(0);
        }
    }
    /* Parent: wait() reaps one child at a time and returns -1 when none are left. */
    while (wait(NULL) > 0) {
    }

    /* Both children are done. Compare the real total with what it should be. */
    printf("expected %ld, got %ld%s\n", 2 * ROUNDS, *counter,
           *counter == 2 * ROUNDS ? "" : "   <- lost updates: a race condition");
    return 0;
}
