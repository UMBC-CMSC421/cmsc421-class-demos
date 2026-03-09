/*
 * Chapter 4: Threads
 * Demo 05: OpenMP hello
 *
 * Build:
 *   gcc -std=c11 -Wall -Wextra -Wpedantic -fopenmp 05_openmp_intro.c -o 05_openmp_intro
 */
#include <omp.h>
#include <stdio.h>

int main(void)
{
    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int total = omp_get_num_threads();
        printf("Hello from thread %d of %d\n", tid, total);
    }

    return 0;
}
