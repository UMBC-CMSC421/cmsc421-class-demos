/*
 * Chapter 3: Processes
 * Demo 02: The other IDs a process carries
 *
 * What's going on
 *   Besides its own PID and its parent's, every process has a few more IDs:
 *     PGID   process group ID. The shell puts each command line you type into its
 *            own group, and a whole pipeline (a | b | c) counts as one group. When
 *            you press Ctrl-C, the signal goes to the whole group, not one process.
 *     SID    session ID. Everything started from the same terminal window shares a
 *            session. Usually the SID is your shell's PID.
 *     UID    which user the process runs as. GID is which group. The "real" one is
 *            who started the process. The "effective" one is what the kernel checks
 *            when the process tries to open a file. They're only different for
 *            setuid programs (demo 15).
 *   This program doesn't create any children. It just prints its own IDs.
 *
 * Build
 *   make
 *
 * Run
 *   ./02_process_attributes
 *
 * What to look for
 *   PID and PGID are the same number, because the shell made this process the leader
 *   of its own group. The SID matches what echo $$ prints in the same terminal. Real
 *   and effective UID are the same, and so are the two GIDs.
 *
 * Other ways to see it
 *   echo | ./02_process_attributes    now it's the second half of a pipeline, so the
 *                                     PGID isn't its own PID anymore. It's the PID of
 *                                     the first half (echo), since the whole pipeline
 *                                     is one group.
 *   id                                your UID and GID. Same numbers as above.
 *   sudo ./02_process_attributes      runs as root, so all the UIDs and GIDs are 0
 *   ps -o pid,ppid,pgid,sid,uid,cmd   the same columns for your shell and for ps
 */
#include <stdio.h>  /* printf(), perror() */
#include <unistd.h> /* getpid(), getsid(), getuid(), and the other ID calls */

int main(void)
{
    /*
     * getsid(0) means "the session ID of me". pid_t is the integer type the system
     * uses for process IDs. Unlike the other calls below, getsid() can fail. When it
     * does, it returns -1, so we check for that.
     */
    pid_t session_id = getsid(0);

    if (session_id < 0) {
        /* perror() prints our label plus the reason the last call failed. */
        perror("getsid");
        return 1; /* a nonzero return from main tells the shell something went wrong */
    }

    /*
     * Each ID gets a (long) cast so it matches %ld, printf's code for a long.
     * The ID types can be different sizes on different systems, and this keeps
     * the format right everywhere.
     */
    printf("Process relationships:\n");
    printf("  PID  = %ld\n", (long)getpid());    /* me */
    printf("  PPID = %ld\n", (long)getppid());   /* my parent */
    printf("  PGID = %ld\n", (long)getpgrp());   /* my process group */
    printf("  SID  = %ld\n", (long)session_id);  /* my session */

    printf("\nProcess credentials:\n");
    printf("  real UID      = %ld\n", (long)getuid());   /* the user who started me */
    printf("  effective UID = %ld\n", (long)geteuid());  /* the user the kernel checks */
    printf("  real GID      = %ld\n", (long)getgid());
    printf("  effective GID = %ld\n", (long)getegid());

    return 0;
}
