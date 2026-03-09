// reentrant_bad_good.c
// Demonstrates non-reentrant code via a shared static buffer
// and how to fix it using caller-provided buffers (reentrant style).
// Build: gcc -O2 reentrant_bad_good.c -o reentrant -Wall
// Run: ./reentrant

#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

static volatile sig_atomic_t fired = 0;

// NON-REENTRANT: returns pointer to a shared static buffer
static const char* make_msg_bad(const char* tag, int x) {
    static char buf[64];                 // shared across calls
    // simulate "work" so interruption is more likely to happen mid-way
    snprintf(buf, sizeof(buf), "%s:%d", tag, x);
    // widen the window where interruption can do damage
    for (volatile int i = 0; i < 20000000; i++) { }
    return buf;
}

// REENTRANT: caller provides buffer (no shared writable state)
static void make_msg_good(char* out, size_t outsz, const char* tag, int x) {
    snprintf(out, outsz, "%s:%d", tag, x);
    for (volatile int i = 0; i < 20000000; i++) { }
}

static void handler(int sig) {
    (void)sig;
    fired = 1;
}

int main(void) {
    // Install a handler and arm a timer signal
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigaction(SIGALRM, &sa, NULL);

    // periodic alarms
    ualarm(20000, 20000); // every 20ms

    puts("BAD (non-reentrant): using shared static buffer");
    for (int i = 0; i < 10; i++) {
        fired = 0;

        const char* a = make_msg_bad("main", i);

        // If a signal fired during make_msg_bad, imagine handler or another path
        // also calling make_msg_bad and overwriting buf.
        // We'll simulate that re-entry immediately when fired == 1.
        if (fired) {
            const char* b = make_msg_bad("signal", 999);
            // Now a and b likely point to the same buf contents
            printf("a=%s   b=%s   (same buffer!)\n", a, b);
        } else {
            printf("a=%s\n", a);
        }
    }

    puts("\nGOOD (reentrant): caller provides buffer");
    for (int i = 0; i < 10; i++) {
        fired = 0;
        char a[64], b[64];

        make_msg_good(a, sizeof(a), "main", i);

        if (fired) {
            make_msg_good(b, sizeof(b), "signal", 999);
            printf("a=%s   b=%s   (independent buffers)\n", a, b);
        } else {
            printf("a=%s\n", a);
        }
    }

    return 0;
}