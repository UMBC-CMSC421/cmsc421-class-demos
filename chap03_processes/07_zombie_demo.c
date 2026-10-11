/*
 * Chapter 3: Processes
 * Demo 07: Zombie processes
 *
 * What's going on
 *   When a process exits, the kernel can't throw all of it away yet. It keeps a
 *   small entry holding the exit status, because the parent might ask for it later.
 *   Until the parent calls wait() or waitpid(), that leftover entry is a zombie.
 *   A zombie is already dead: no user memory, no code running, just a small kernel
 *   record holding its PID and exit status.
 *   Here the child exits right away, but the parent sleeps 10 seconds before calling
 *   waitpid(). That gives you 10 seconds to catch the zombie.
 *
 * Build
 *   make
 *
 * Run
 *   Terminal 1:   ./07_zombie_demo
 *   Terminal 2:   ps -o pid,ppid,stat,cmd -C 07_zombie_demo      (within 10 seconds)
 *   -C picks processes by program name, so it works from any terminal.
 *
 *   Only have one terminal? Put it in the background with &, then run the ps:
 *     ./07_zombie_demo &
 *     ps -o pid,ppid,stat,cmd -C 07_zombie_demo
 *
 * What to look for
 *   Two rows. The parent has STAT S (sleeping). The child has STAT Z, with <defunct>
 *   after its name, and its PPID is the parent. If terminal 1 runs the demo in the
 *   foreground, you'll see S+ and Z+: the + means "in the foreground of its terminal".
 *   When the 10 seconds are up, the parent reaps it: the Z row disappears and the
 *   parent prints exit status 42.
 *
 * Other ways to see it (in terminal 2, during the 10 seconds)
 *   top                                    the "Tasks:" line counts zombies. q quits.
 *   grep State /proc/<child PID>/status    says "Z (zombie)"
 *   kill -9 <child PID>                    does nothing. You can't kill something
 *                                          that's already dead. Run ps again to check.
 *   kill <parent PID>                      now the zombie goes away too. With its parent
 *                                          gone it gets adopted, usually by PID 1,
 *                                          which reaps it right away (demo 19).
 */
#include <stdio.h>     /* printf(), perror() */
#include <stdlib.h>
#include <sys/types.h> /* pid_t */
#include <sys/wait.h>  /* waitpid(), WIFEXITED, WEXITSTATUS */
#include <unistd.h>    /* fork(), sleep(), _exit() */

int main(void)
{
    /*
     * fork() makes the child. pid_t is the integer type for process IDs. The child
     * gets 0 back, the parent gets the child's PID, and -1 means it failed.
     */
    pid_t pid = fork();

    if (pid < 0)
    {
        /* perror() prints our label plus the reason the call failed. */
        perror("fork");
        return 1;
    }

    if (pid == 0)
    {
        /* Child: say hi and exit right away. From here on it's a zombie until reaped. */
        printf("[child ] exiting immediately; parent PID=%ld\n", (long)getppid());
        fflush(stdout); /* push the line out now, before the child ends */
        /*
         * _exit() ends the child right away (demo 04 explains why not exit()).
         * 42 is just a number we'll recognize when the parent reads it.
         */
        _exit(42);
    }

    printf("[parent] child PID=%ld created\n", (long)pid);
    printf("[parent] sleeping for 10 seconds before waitpid(); observe the zombie now\n");
    fflush(stdout);

    /* The parent is busy sleeping, not waiting, so nobody reaps the child yet. */
    sleep(10);

    /*
     * Now reap it. This is the moment the zombie entry gets removed. We pass &status
     * (the address of status) so waitpid() can write the child's exit info into it.
     */
    int status = 0;
    if (waitpid(pid, &status, 0) < 0)
    {
        perror("waitpid");
        return 1;
    }

    /* WIFEXITED: did it exit normally? If so, WEXITSTATUS pulls out the exit code. */
    if (WIFEXITED(status))
    {
        printf("[parent] reaped child; exit status=%d\n", WEXITSTATUS(status));
    }

    return 0;
}
