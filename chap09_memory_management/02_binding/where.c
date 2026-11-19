/*
 * Chapter 9: Main Memory
 * Demo 02, where.c: print the run-time (virtual) addresses of a function and a global.
 *
 * Day 1 commands (run in this folder on the course VM):
 *   gcc -Wall -o where where.c
 *   ./where ; ./where            two runs, two different addresses (ASLR)
 *   nm where | grep -E ' (main|counter)$'   the linker's offsets never change
 *   cat /proc/sys/kernel/randomize_va_space  2 means full randomization
 *
 * address printed = base chosen by execve at load time + offset fixed by the linker
 */
#include <stdio.h>

int counter = 0;    /* a global variable: lives in the .bss region */

int main(void)
{
    /* %p prints a pointer in hex. Casting the function pointer to void * is
     * allowed by POSIX (dlsym relies on it), so the build uses no -Wpedantic. */
    printf("main    at %p\n", (void *)main);
    printf("counter at %p\n", (void *)&counter);
    return 0;
}
