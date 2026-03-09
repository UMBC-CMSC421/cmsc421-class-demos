/*
 * Chapter 3: Processes
 * Demo 07: Signal handling
 *
 * What this demo shows:
 * - A process can install handlers for some signals.
 * - SIGINT can be sent with Ctrl-C.
 * - SIGUSR1 can be sent from another terminal using: kill -USR1 <pid>
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static volatile sig_atomic_t sigint_count = 0;
static volatile sig_atomic_t sigusr1_seen = 0;

static void handle_sigint(int signo)
{
    (void)signo;
    sigint_count++;
}

static void handle_sigusr1(int signo)
{
    (void)signo;
    sigusr1_seen = 1;
}

int main(void)
{
    struct sigaction sa_int;
    struct sigaction sa_usr1;

    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;

    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;

    if (sigaction(SIGINT, &sa_int, NULL) < 0) {
        perror("sigaction SIGINT");
        return 1;
    }

    if (sigaction(SIGUSR1, &sa_usr1, NULL) < 0) {
        perror("sigaction SIGUSR1");
        return 1;
    }

    printf("PID=%ld\n", (long)getpid());
    printf("Press Ctrl-C or send SIGUSR1 with: kill -USR1 %ld\n", (long)getpid());

    while (sigint_count < 3) {
        pause();

        printf("Observed signals so far: SIGINT=%d, SIGUSR1=%d\n",
               (int)sigint_count,
               (int)sigusr1_seen);
    }

    printf("Exiting after 3 SIGINT deliveries.\n");
    return 0;
}
