#ifndef WALKTHROUGH_H
#define WALKTHROUGH_H

/* Shared command-line handling for the Chapter 9 walkthroughs.
 * Read each demo's main() for its teaching sequence. */
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    bool step;
    bool has_address;
    uint32_t address;
} Options;

/* Accept decimal or 0x-prefixed addresses. Reject signs, trailing text, and
 * values wider than the 32-bit address space used in these models. */
static inline Options options(int argc, char **argv, bool allow_address)
{
    Options out = {false, false, 0};
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--step") == 0 && !out.step) {
            out.step = true;
            continue;
        }
        char *end = NULL;
        const char *s = argv[i];
        if (!allow_address || out.has_address || s[0] < '0' || s[0] > '9') {
            fprintf(stderr, "Usage: %s [--step]%s\n", argv[0], allow_address ? " [address]" : "");
            exit(EXIT_FAILURE);
        }
        errno = 0;
        int base = s[0] == '0' && (s[1] == 'x' || s[1] == 'X') ? 16 : 10;
        unsigned long long n = strtoull(s, &end, base);
        if (errno != 0 || end == s || *end != '\0' || n > UINT32_MAX) {
            fprintf(stderr, "Invalid 32-bit address: %s\n", s);
            exit(EXIT_FAILURE);
        }
        out.address = (uint32_t)n;
        out.has_address = true;
    }
    return out;
}

/* In lecture mode, stop before revealing the next step. EOF lets a piped run
 * finish without waiting; the default mode never reads from the terminal. */
static inline void pause_step(bool step)
{
    if (!step) return;
    fputs("Press Enter to continue...", stdout);
    fflush(stdout);
    int c;
    do { c = getchar(); } while (c != '\n' && c != EOF);
    putchar('\n');
}
#endif
