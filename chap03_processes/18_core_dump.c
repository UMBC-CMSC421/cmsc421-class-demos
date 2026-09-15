/*
 * Chapter 3: Processes
 * Demo 18: Crashing, core dumps, and finding the bug with gdb
 *
 * What's going on
 *   This program has a bug on purpose. On the third score it reads through a NULL
 *   pointer. Nothing is ever mapped at address 0, so the CPU faults and the kernel
 *   kills the process with SIGSEGV, the "segmentation fault" you've probably seen.
 *   When that happens, the kernel can also save a core dump: a file holding the
 *   process's memory at the moment it died (Fox 4.6.1). A debugger can open that
 *   file later and show exactly which line crashed and what the variables were.
 *   Core dumps are off by default, because they can get big.
 *
 * Build
 *   make          (the Makefile adds -g for this demo, so gdb can show line numbers)
 *
 * Run
 *   ulimit -c unlimited      allow core files. It only lasts for this terminal window.
 *   ./18_core_dump
 *   echo $?                  139, which is 128 + 11, and 11 is SIGSEGV
 *   ls -l core               there's the core dump
 *   gdb -q -batch -ex bt -ex 'frame 1' -ex 'info locals' ./18_core_dump core
 *   rm -f core               cores take up space, so delete it when you're done
 *
 * What to look for
 *   bt (backtrace) lists the function calls that were in progress at the crash:
 *   load_score, called from total_scores, called from main. "frame 1" moves up to
 *   total_scores, and "info locals" shows i = 2 and slot = 0x0. There's the NULL.
 *
 * Other ways to see it
 *   You don't need a core file to debug a crash. You can run the program inside gdb:
 *     gdb -q ./18_core_dump
 *     (gdb) run                    runs until it crashes
 *     (gdb) bt
 *     (gdb) frame 1
 *     (gdb) print i
 *     (gdb) print scores[i]        77, the value it should have read
 *     (gdb) quit
 *   cat /proc/sys/kernel/core_pattern
 *                                  where core files go. "core" means a file named
 *                                  core in the current directory.
 *   kill -l 139                    SEGV
 */
#include <stdio.h>
#include <sys/resource.h>
#include <unistd.h>

static int load_score(const int *slot)
{
    return *slot; /* slot is NULL on the third call: invalid address -> SIGSEGV */
}

static int total_scores(const int *scores, int count)
{
    int total = 0;

    for (int i = 0; i < count; i++) {
        /* Here's the bug: when i is 2 we pass NULL instead of the real address. */
        const int *slot = (i == 2) ? NULL : &scores[i];
        total += load_score(slot);
    }
    return total;
}

int main(void)
{
    const int scores[4] = {90, 85, 77, 64};
    struct rlimit core_limit;

    /*
     * RLIMIT_CORE is the biggest core file this process is allowed to write. 0 means
     * no core file at all. It's the same setting "ulimit -c" changes in your shell.
     */
    if (getrlimit(RLIMIT_CORE, &core_limit) == 0) {
        if (core_limit.rlim_cur == 0) {
            printf("Core dumps are off in this shell (ulimit -c is 0). Run: ulimit -c unlimited\n");
        } else if (core_limit.rlim_cur == RLIM_INFINITY) {
            printf("Core dump size limit: unlimited\n");
        } else {
            printf("Core dump size limit: %llu bytes\n", (unsigned long long)core_limit.rlim_cur);
        }
    }

    printf("PID %ld adding up scores; the third one is read through a NULL pointer...\n", (long)getpid());
    fflush(stdout); /* print this now, because the crash would throw away anything still buffered */

    printf("total = %d\n", total_scores(scores, 4)); /* we never get to print this */
    return 0;
}
