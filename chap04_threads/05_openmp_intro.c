/*
 * Chapter 4: Threads
 * Demo 05: OpenMP hello
 *
 * What's going on
 *   OpenMP is implicit threading: you mark a block of code, and a runtime library
 *   creates the threads, runs the block in each one, and waits for them all. The
 *   line #pragma omp parallel is the marker. There's no pthread_create() or
 *   pthread_join() anywhere in this file.
 *
 *   The runtime picks how many threads to use. By default that's usually one per
 *   CPU. The OMP_NUM_THREADS environment variable sets it yourself.
 *
 * Build
 *   make 05_openmp_intro   (or: make openmp)
 *   The Makefile adds -fopenmp. Without that flag the program doesn't link.
 *
 * Run
 *   ./05_openmp_intro
 *   OMP_NUM_THREADS=2 ./05_openmp_intro
 *
 * What to look for
 *   Before and after the parallel block, main is alone: thread 0 in a team of 1.
 *   Inside it, one [worker N] pair per thread. The course VM has 4 CPUs, so you get
 *   workers 0 to 3, in a different order each run. Every worker prints the same PID,
 *   because they're all threads of one process.
 */
#include <omp.h>    /* omp_get_thread_num(), omp_get_num_threads(), and friends */
#include <stdio.h>  /* printf(), setbuf() */
#include <stdlib.h> /* getenv() */
#include <unistd.h> /* getpid() */

int main(void)
{
    /* getenv() returns NULL when the variable isn't set. */
    const char *requested = getenv("OMP_NUM_THREADS");

    /* Turn off output buffering so every line appears the moment it's printed. */
    setbuf(stdout, NULL);
    printf("=== Demo 05: OpenMP creates a runtime-managed team ===\n");
    printf("[step 1] before the parallel region: PID=%ld OpenMP thread=%d team=%d\n",
           (long)getpid(), omp_get_thread_num(), omp_get_num_threads());
    printf("[step 1] OMP_NUM_THREADS=%s; runtime maximum=%d; available processors=%d\n",
           requested != NULL ? requested : "not set",
           omp_get_max_threads(), omp_get_num_procs());
    printf("[step 2] main reaches '#pragma omp parallel'\n");

    /* The runtime starts the team here. Every thread runs the block in braces. */
    #pragma omp parallel
    {
        /* Declared inside the block, so each thread has its own copy of these. */
        int tid = omp_get_thread_num();    /* this thread's number in the team */
        int total = omp_get_num_threads(); /* how many threads are in the team */

        printf("[worker %d] entered region: PID=%ld team size=%d in_parallel=%d\n",
               tid, (long)getpid(), total, omp_in_parallel());
        printf("[worker %d] executing the same structured block\n", tid);
    }
    /* The closing brace is an implicit barrier: main waits here for every thread. */

    printf("[step 3] implicit barrier completed; every worker has left the region\n");
    printf("[step 3] main continues alone: OpenMP thread=%d team=%d\n",
           omp_get_thread_num(), omp_get_num_threads());
    printf("[result] no pthread_create() or pthread_join() appeared in our source; the runtime managed them\n");
    return 0;
}
