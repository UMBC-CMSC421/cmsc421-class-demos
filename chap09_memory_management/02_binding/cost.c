/*
 * Chapter 9: Main Memory
 * Demo 02, cost.c: what one memory access and one trip into the kernel cost.
 *
 * 1. Cache hit: follow a chain of pointers through a 16 KiB array, small enough
 *    to stay in the L1 cache.
 * 2. RAM: follow a random chain through a 512 MiB array, so almost every step
 *    misses every cache and goes to main memory.
 * 3. Kernel: call getpid() through syscall(), so every call enters the kernel
 *    and comes back.
 *
 * Build: gcc -O2 -Wall -o cost cost.c    (or: make)
 * Run:   ./cost                          needs about 1 GiB of free memory
 *
 * The numbers change from machine to machine and run to run. The ratios are
 * the point: a kernel round trip costs as much as many cache hits.
 */
#define _GNU_SOURCE             /* exposes syscall() in unistd.h */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>

/* The current time in nanoseconds, from a clock that never jumps backward. */
static double now_ns(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec * 1e9 + (double)t.tv_nsec;
}

/* A random number from 0 to limit - 1, built from two rand() calls so it can
 * exceed RAND_MAX when the array is large. */
static size_t random_below(size_t limit)
{
    size_t r = (size_t)rand() * ((size_t)RAND_MAX + 1) + (size_t)rand();
    return r % limit;
}

/* Build one random cycle through n slots, then time 'steps' pointer hops.
 * Returns the average time of one hop in nanoseconds. */
static double chase(size_t n, long steps)
{
    size_t *next = malloc(n * sizeof *next);    /* next[i]: the slot to visit after i */
    size_t *order = malloc(n * sizeof *order);  /* the visiting order, shuffled */
    if (next == NULL || order == NULL) {
        perror("malloc");
        exit(1);
    }
    for (size_t i = 0; i < n; i++)
        order[i] = i;
    srand(421);                                  /* same shuffle every run */
    for (size_t i = n - 1; i > 0; i--) {         /* Fisher-Yates shuffle */
        size_t j = random_below(i + 1);
        size_t t = order[i];
        order[i] = order[j];
        order[j] = t;
    }
    for (size_t i = 0; i < n; i++)               /* link each slot to the next one in order */
        next[order[i]] = order[(i + 1) % n];

    size_t p = order[0];
    for (long i = 0; i < steps / 10; i++)        /* warm up: load the caches and the TLB */
        p = next[p];
    double t0 = now_ns();
    for (long i = 0; i < steps; i++)
        p = next[p];                             /* each hop needs the last one's result */
    double t1 = now_ns();
    if (p == (size_t)-1)
        puts("");                                /* uses p, so the compiler keeps the loop */
    free(next);
    free(order);
    return (t1 - t0) / (double)steps;
}

int main(void)
{
    double hit = chase(16 * 1024 / sizeof(size_t), 100000000L);
    double ram = chase(512UL * 1024 * 1024 / sizeof(size_t), 20000000L);

    long calls = 5000000L;
    double t0 = now_ns();
    for (long i = 0; i < calls; i++)
        syscall(SYS_getpid);                     /* user mode -> kernel mode -> user mode */
    double sys = (now_ns() - t0) / (double)calls;

    printf("cache hit (16 KiB chain)    %6.1f ns per access\n", hit);
    printf("RAM (512 MiB random chain)  %6.1f ns per access\n", ram);
    printf("kernel round trip (getpid)  %6.1f ns per call\n", sys);
    return 0;
}
