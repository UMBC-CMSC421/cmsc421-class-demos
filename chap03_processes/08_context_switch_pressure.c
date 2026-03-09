/*
 * Chapter 3: Processes
 * Demo 08: Context-switch pressure
 *
 * What this demo shows:
 * - Creating many short-lived child processes causes scheduling overhead.
 * - This is not a precise benchmark; it is a discussion starter.
 *
 * Try:
 *   time ./08_context_switch_pressure 50
 *   time ./08_context_switch_pressure 500
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static long parse_count(const char *s)
{
    char *end = NULL;
    long value = strtol(s, &end, 10);

    if (end == s || *end != '\0' || value <= 0) {
        fprintf(stderr, "Invalid child count: %s\n", s);
        exit(1);
    }

    return value;
}

int main(int argc, char *argv[])
{
    long count = 100;

    if (argc == 2) {
        count = parse_count(argv[1]);
    }

    for (long i = 0; i < count; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            return 1;
        }

        if (pid == 0) {
            _exit(0);
        }
    }

    for (long i = 0; i < count; i++) {
        if (wait(NULL) < 0) {
            perror("wait");
            return 1;
        }
    }

    printf("Created and reaped %ld short-lived children.\n", count);
    return 0;
}
