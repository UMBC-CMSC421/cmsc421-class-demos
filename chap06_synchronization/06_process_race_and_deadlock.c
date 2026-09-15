/*
 * Chapter 6: Synchronization Tools
 * Demo 06: Race conditions and deadlock between processes
 *
 * What this demo shows (Fox 14.4.1, liveness and starvation):
 * - Two processes that read, change, and write the same file without coordinating
 *   lose each other's updates.
 * - A lock makes each read-change-write happen alone, so nothing is lost.
 * - Two processes that each hold one lock and wait for the other's lock are deadlocked.
 *   Fox says Linux ignores deadlock for user processes. For fcntl() record locks the
 *   kernel actually detects the cycle and refuses one request with EDEADLK; for flock()
 *   it does not, and both processes would wait forever.
 *
 * Run:
 *   ./06_process_race_and_deadlock
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ITERATIONS 2000

static void nap_ms(long milliseconds)
{
    struct timespec delay = {.tv_sec = milliseconds / 1000, .tv_nsec = (milliseconds % 1000) * 1000000L};
    (void)nanosleep(&delay, NULL);
}

static void write_text(const char *path, const char *text)
{
    FILE *file = fopen(path, "w");

    if (file == NULL) {
        perror(path);
        exit(1);
    }
    (void)fputs(text, file);
    (void)fclose(file);
}

/* Read the number in the file, add one, write it back -- ITERATIONS times. */
static void increment_many(const char *path, int use_lock)
{
    for (int i = 0; i < ITERATIONS; i++) {
        char text[32] = {0};
        int fd = open(path, O_RDWR);

        if (fd < 0 || (use_lock && flock(fd, LOCK_EX) < 0)) {
            perror("open/flock");
            _exit(1);
        }
        ssize_t got = pread(fd, text, sizeof(text) - 1, 0);
        long value = got > 0 ? strtol(text, NULL, 10) : 0;
        int length = snprintf(text, sizeof(text), "%ld\n", value + 1);
        if (pwrite(fd, text, (size_t)length, 0) < 0) {
            perror("pwrite");
            _exit(1);
        }
        (void)close(fd); /* closing the file also releases the flock */
    }
    _exit(0);
}

static long race(const char *path, int use_lock)
{
    long value = -1;
    FILE *file;

    write_text(path, "0\n");
    pid_t first = fork();
    if (first == 0) {
        increment_many(path, use_lock);
    }
    pid_t second = fork();
    if (second == 0) {
        increment_many(path, use_lock);
    }
    (void)waitpid(first, NULL, 0);
    (void)waitpid(second, NULL, 0);

    file = fopen(path, "r");
    if (file != NULL) {
        if (fscanf(file, "%ld", &value) != 1) {
            value = -1;
        }
        (void)fclose(file);
    }
    return value;
}

static int lock_byte(int fd, off_t byte)
{
    struct flock region = {0};

    region.l_type = F_WRLCK;
    region.l_whence = SEEK_SET;
    region.l_start = byte;
    region.l_len = 1;
    return fcntl(fd, F_SETLKW, &region);
}

/* fcntl locks: take byte `first`, pause so the other process takes its byte, then want `second`. */
static void fcntl_in_order(const char *path, off_t first, off_t second, const char *name)
{
    int fd = open(path, O_RDWR);

    if (fd < 0 || lock_byte(fd, first) < 0) {
        perror("fcntl lock");
        _exit(1);
    }
    printf("  %s (PID %ld) holds byte %ld and now waits for byte %ld\n",
           name, (long)getpid(), (long)first, (long)second);
    fflush(stdout);
    nap_ms(300);
    if (lock_byte(fd, second) < 0) {
        printf("  %s: fcntl() failed with %s -- the kernel found the cycle and refused\n",
               name, errno == EDEADLK ? "EDEADLK" : strerror(errno));
        fflush(stdout);
        _exit(1);
    }
    printf("  %s: got both bytes (the other process gave up its lock)\n", name);
    fflush(stdout);
    _exit(0);
}

static void handle_alarm(int signo)
{
    (void)signo;
}

/* flock locks: the same pattern, with a 3-second alarm so the demo can end. */
static void flock_in_order(const char *first, const char *second, const char *name)
{
    struct sigaction sa;
    int first_fd = open(first, O_RDONLY);
    int second_fd = open(second, O_RDONLY);

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_alarm;
    sigemptyset(&sa.sa_mask);
    (void)sigaction(SIGALRM, &sa, NULL); /* no SA_RESTART: the alarm interrupts flock() */

    if (first_fd < 0 || second_fd < 0 || flock(first_fd, LOCK_EX) < 0) {
        perror("flock");
        _exit(1);
    }
    printf("  %s (PID %ld) holds %s and now waits for %s\n", name, (long)getpid(),
           strrchr(first, '/') + 1, strrchr(second, '/') + 1);
    fflush(stdout);
    nap_ms(300);
    alarm(3);
    if (flock(second_fd, LOCK_EX) < 0 && errno == EINTR) {
        printf("  %s: still waiting after 3 seconds -- deadlocked; flock() never detects it\n", name);
        fflush(stdout);
        _exit(1);
    }
    printf("  %s: got both locks, but only because the other process gave up\n", name);
    fflush(stdout);
    _exit(0);
}

int main(void)
{
    char counter[64];
    char lock_a[64];
    char lock_b[64];

    (void)snprintf(counter, sizeof(counter), "/tmp/cmsc421_counter_%ld", (long)getpid());
    (void)snprintf(lock_a, sizeof(lock_a), "/tmp/cmsc421_lock_A_%ld", (long)getpid());
    (void)snprintf(lock_b, sizeof(lock_b), "/tmp/cmsc421_lock_B_%ld", (long)getpid());

    printf("1) Two processes each add 1 to a number in a file %d times. Correct total: %d\n",
           ITERATIONS, 2 * ITERATIONS);
    fflush(stdout);
    printf("   without a lock: %ld\n", race(counter, 0));
    fflush(stdout);
    printf("   with flock():   %ld\n\n", race(counter, 1));

    printf("2) Deadlock with fcntl() record locks (bytes 0 and 1 of one file):\n");
    fflush(stdout);
    write_text(counter, "xx");
    pid_t first = fork();
    if (first == 0) {
        fcntl_in_order(counter, 0, 1, "process A");
    }
    pid_t second = fork();
    if (second == 0) {
        nap_ms(50);
        fcntl_in_order(counter, 1, 0, "process B");
    }
    (void)waitpid(first, NULL, 0);
    (void)waitpid(second, NULL, 0);

    printf("\n3) The same deadlock with flock() on two files:\n");
    fflush(stdout);
    write_text(lock_a, "");
    write_text(lock_b, "");
    first = fork();
    if (first == 0) {
        flock_in_order(lock_a, lock_b, "process A");
    }
    second = fork();
    if (second == 0) {
        nap_ms(50);
        flock_in_order(lock_b, lock_a, "process B");
    }
    (void)waitpid(first, NULL, 0);
    (void)waitpid(second, NULL, 0);

    (void)unlink(counter);
    (void)unlink(lock_a);
    (void)unlink(lock_b);
    return 0;
}
