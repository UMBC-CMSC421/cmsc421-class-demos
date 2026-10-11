/*
 * Chapter 3: Processes
 * Demo 01: Who am I? (PID and PPID)
 *
 * What's going on
 *   When you type ./01_proc_identity, your shell starts a new process to run it.
 *   The kernel gives that process a number, its PID (process ID). It also writes
 *   down who started it. That's the PPID (parent process ID), and here the parent
 *   is your shell.
 *
 * Build
 *   make                      builds every demo in this folder
 *
 * Run
 *   ./01_proc_identity
 *
 * What to look for
 *   Run it three times in a row. The PID changes every time, because every run is
 *   a brand new process. The PPID stays the same, because the same shell started
 *   all three.
 *
 * Other ways to see it
 *   echo $$                   your shell's own PID. Same number as the PPID above.
 *   ps -o pid,ppid,cmd        your shell, plus the ps you just ran. ps's PPID is
 *                             your shell too.
 */
#include <stdio.h>  /* printf() */
#include <unistd.h> /* getpid(), getppid() */

int main(void)
{
    printf("One process is running.\n");

    /*
     * getpid() asks the kernel "what's my PID?" and getppid() asks "who started me?".
     * Both are system calls: your program stops for a moment, the kernel looks up
     * the answer in its record for this process, and hands the number back.
     *
     * They return a pid_t, an integer type whose size depends on the system. The
     * (long) in front converts the value to a long, and %ld is printf's code for a
     * long. That way the format always matches.
     */
    printf("Process ID (PID)        = %ld\n", (long)getpid());
    printf("Parent process ID (PPID) = %ld\n", (long)getppid());

    /* Returning 0 tells the shell "this worked". Run echo $? right after to see it. */
    return 0;
}
