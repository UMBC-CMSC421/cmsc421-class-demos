/*
 * Chapter 10: Virtual Memory
 * Demo 03: virtual size vs. resident size.
 *
 * Maps 4 GiB of anonymous memory, then touches 1 page, then 256 MiB.
 * After each step it prints VmSize (virtual) and VmRSS (resident) from
 * /proc/self/status and the minor/major fault counts from getrusage().
 *
 * The course VM sets transparent huge pages to "always", so the kernel would
 * back this region with 2 MiB pages and take one fault per 2 MiB, not per
 * 4 KiB page. madvise(MADV_NOHUGEPAGE) turns that off for this region only,
 * so the counts show one minor fault per 4 KiB page, as in lecture.
 *
 * Build: make   (builds 03_lazy_alloc)
 * Run:   ./03_lazy_alloc
 */
#define _DEFAULT_SOURCE /* exposes MAP_ANONYMOUS under -std=c11 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

static void report(const char *label)
{
    char line[256];
    long vsz = 0, rss = 0;
    FILE *f = fopen("/proc/self/status", "r");
    while (f && fgets(line, sizeof line, f)) {
        sscanf(line, "VmSize: %ld kB", &vsz);
        sscanf(line, "VmRSS: %ld kB", &rss);
    }
    if (f)
        fclose(f);
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    printf("%-16s VSZ %5ld MiB  RSS %4ld MiB  minflt %6ld  majflt %ld\n",
           label, vsz / 1024, rss / 1024, ru.ru_minflt, ru.ru_majflt);
}

int main(void)
{
    const size_t GiB = 1UL << 30, MiB = 1UL << 20;
    size_t page = (size_t)sysconf(_SC_PAGESIZE);

    report("start");
    char *p = mmap(NULL, 4 * GiB, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (p == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    /* Ask for 4 KiB pages in this region (see the note at the top). */
    if (madvise(p, 4 * GiB, MADV_NOHUGEPAGE) != 0)
        perror("madvise");                      /* not fatal: counts are just smaller */
    report("mapped 4 GiB");

    p[0] = 1;                                   /* touch one page */
    report("touched 1 page");

    for (size_t off = 0; off < 256 * MiB; off += page)
        p[off] = 1;                             /* touch 65,536 pages */
    report("touched 256 MiB");

    munmap(p, 4 * GiB);
    return 0;
}
