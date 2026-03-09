/*
 * Chapter 3: Processes
 * Demo 01: Process identity
 *
 * What this demo shows:
 * - Every process has a PID and a parent PID.
 * - Command-line arguments are provided through argv.
 * - Environment variables are visible in process context.
 *
 * Build:
 *   gcc -std=c11 -Wall -Wextra -Wpedantic 01_proc_identity.c -o 01_proc_identity
 *
 * Run:
 *   ./01_proc_identity hello world
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int i;
    const char *path = getenv("PATH");

    printf("PID  = %ld\n", (long)getpid());
    printf("PPID = %ld\n", (long)getppid());
    printf("argc = %d\n", argc);

    for (i = 0; i < argc; i++) {
        printf("argv[%d] = \"%s\"\n", i, argv[i]);
    }

    if (path != NULL) {
        printf("PATH starts with: %.60s%s\n", path, path[60] ? "..." : "");
    } else {
        printf("PATH is not set in the environment.\n");
    }

    return 0;
}
