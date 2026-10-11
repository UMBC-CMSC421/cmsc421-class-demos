/*
 * Chapter 3: Processes
 * Demo 31: a pidfd, a file descriptor that names one process
 *
 * What's going on
 *   A PID is just a number, and the kernel reuses numbers. If a parent saves a
 *   PID, the child exits and is reaped, and a new process gets the same number,
 *   kill(pid, ...) hits the wrong process. Linux added pidfds to fix this:
 *     pidfd_open(pid, 0)          Linux 5.3   a file descriptor for that process
 *     poll() on the pidfd         Linux 5.3   readable when the process exits
 *     pidfd_send_signal(fd, ...)  Linux 5.1   signal exactly that process
 *     waitid(P_PIDFD, fd, ...)    Linux 5.4   reap it through the pidfd
 *   The course kernel (6.1) has all of them. This demo starts a child, opens a
 *   pidfd for it, waits for it to exit with poll(), and reaps it with waitid().
 *
 *   A pidfd always refers to the same process, even after its PID number gets
 *   reused. You hold it the same way you hold an open file.
 *
 * Build
 *   make 31_pidfd_wait
 *
 * Run
 *   ./31_pidfd_wait
 *
 * What to look for
 *   "pidfd N is readable": the kernel told us the child exited, with no SIGCHLD
 *   handler and no polling of PIDs. Then waitid reports the exit code, 7.
 *   ls -l /proc/<PID>/fd while it sleeps shows the pidfd as anon_inode:[pidfd]
 *   (on 6.9 and later pidfds live in pidfs, but the link name is kept the same).
 *
 * Other ways to see it
 *   man 2 pidfd_open, man 2 pidfd_send_signal, man 2 waitid
 */
#define _GNU_SOURCE
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* Older C library headers may not define P_PIDFD yet. 3 is the kernel's value. */
#ifndef P_PIDFD
#define P_PIDFD 3
#endif

int main(void)
{
    pid_t pid;            /* pid_t is the type the system uses for process IDs */
    int pidfd;
    struct pollfd pfd;    /* tells poll() which fd to watch and for what */
    siginfo_t info;       /* waitid() fills this in with how the child ended */

    fflush(stdout); /* empty stdout's buffer before fork so nothing prints twice */
    pid = fork();
    if (pid < 0) {
        perror("fork"); /* perror prints your text plus the reason the call failed */
        return 1;
    }
    if (pid == 0) {
        /* Child: nap for 2 seconds, then exit with code 7 so the parent has
           something to report. _exit() ends the child right away. */
        sleep(2);
        _exit(7);
    }

    /* Parent: get a pidfd for the child. syscall() makes the raw system call by
       its number, SYS_pidfd_open, so we don't depend on a C library wrapper.
       It returns a long, so we cast it to (int) to store it as a file descriptor. */
    pidfd = (int)syscall(SYS_pidfd_open, pid, 0);
    if (pidfd < 0) {
        perror("pidfd_open (needs Linux 5.3 or later)");
        return 1;
    }
    printf("parent: child PID %d, pidfd %d; waiting with poll()\n", (int)pid, pidfd);

    /* Ask poll() to wake us when the pidfd becomes readable (POLLIN).
       For a pidfd, "readable" means the process has exited.
       The 1 means we're watching one fd, and -1 means wait as long as it takes. */
    pfd.fd = pidfd;
    pfd.events = POLLIN;
    if (poll(&pfd, 1, -1) < 0) {
        perror("poll");
        return 1;
    }
    printf("parent: pidfd %d is readable: the child has exited\n", pidfd);

    /* Reap the child through the pidfd instead of by PID. WEXITED means
       "wait for a child that has exited." The casts give waitid the types it wants. */
    if (waitid((idtype_t)P_PIDFD, (id_t)pidfd, &info, WEXITED) < 0) {
        perror("waitid P_PIDFD (needs Linux 5.4 or later)");
        return 1;
    }
    /* si_pid is who ended, and si_status is its exit code. */
    printf("parent: reaped PID %d, exit code %d\n", (int)info.si_pid, info.si_status);
    close(pidfd); /* a pidfd is a file descriptor, so close it when you're done */
    return 0;
}
