/*
 * Chapter 10: Virtual Memory
 * Demo 04: copy-on-write after fork().
 *
 * The parent fills 64 MiB (16,384 pages), then forks. The child counts
 * its own minor faults while it reads every page, writes one page, and
 * then writes every page. Reads share the parent's frames; each first
 * write to a shared page is a copy-on-write fault.
 *
 * Build: make   (builds 04_cow_fork)
 * Run:   ./04_cow_fork
 */
#define _DEFAULT_SOURCE /* exposes MAP_ANONYMOUS under -std=c11 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

static long minflt(void)
{
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return ru.ru_minflt;
}

int main(void)
{
    const size_t size = 64UL << 20;
    size_t page = (size_t)sysconf(_SC_PAGESIZE);
    volatile char *buf = malloc(size);
    if (!buf)
        return 1;
    memset((char *)buf, 'P', size);             /* parent owns every page */

    pid_t pid = fork();
    if (pid == 0) {
        long sum = 0, before = minflt();
        for (size_t off = 0; off < size; off += page)
            sum += buf[off];                    /* read: frames stay shared */
        long after_read = minflt();

        buf[0] = 'C';                           /* first write: one COW copy */
        long after_one = minflt();

        for (size_t off = 0; off < size; off += page)
            buf[off] = 'C';                     /* write every page */
        long after_all = minflt();

        printf("pages in buffer      : %zu\n", size / page);
        printf("child read all pages : %5ld new faults\n", after_read - before);
        printf("child wrote 1 page   : %5ld new faults\n", after_one - after_read);
        printf("child wrote the rest : %5ld new faults\n", after_all - after_one);
        (void)sum;
        fflush(stdout);
        _exit(0);
    }
    waitpid(pid, NULL, 0);
    printf("parent buf[0] is still '%c'\n", buf[0]);
    return 0;
}
