// interrupts_user.c: can a regular program turn off interrupts? Nope.
// DAY 1 SLIDES 22-24. This one doesn't touch the counter.
// Turning off interrupts is a privileged instruction (cli on x86, msr daifset on Arm).
// Try it in user mode and the CPU says no, and the kernel kills the program.
// Even the kernel can only turn them off on one CPU. The other CPUs keep going.
// EXPECT: a "CPU refused" message, and exit status 1.
// BUILD: gcc -O2 interrupts_user.c -o interrupts_user
// RUN:   ./interrupts_user
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void refused(int sig) {
    const char *msg = sig == SIGSEGV
        ? "CPU refused: SIGSEGV (general protection fault). User programs cannot disable interrupts.\n"
        : "CPU refused: SIGILL (illegal instruction). User programs cannot disable interrupts.\n";
    if (write(STDOUT_FILENO, msg, strlen(msg)) < 0) _exit(2);
    _exit(1);
}

int main(void) {
    signal(SIGSEGV, refused);
    signal(SIGILL, refused);
    printf("Trying to disable interrupts from user mode...\n");
    fflush(stdout);
#if defined(__x86_64__) || defined(__i386__)
    __asm__ volatile("cli");          // x86: clear the interrupt flag
#elif defined(__aarch64__)
    __asm__ volatile("msr daifset, #2"); // Arm: mask IRQs
#else
#error "x86 or Arm only"
#endif
    printf("Interrupts are off (this line should never print).\n");
    return 0;
}
