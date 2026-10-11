/*
 * Chapter 3: Processes
 * Demo 15: Real vs effective user ID, and the setuid bit
 *
 * What's going on
 *   Every process carries more than one user ID:
 *     real UID        who started the process (you)
 *     effective UID   whose permissions the kernel checks when the process opens a
 *                     file, sends a signal, and so on
 *     saved UID       a spare copy, so a program can switch back and forth
 *   Normally all three are you. But if the program FILE has the setuid bit turned on
 *   (ls -l shows an s, like rwsr-xr-x), then running it sets the effective UID to the
 *   file's owner. That's how passwd, which is owned by root, can write your new
 *   password into /etc/shadow, a file you can't even read (Fox 4.3.1).
 *   A careful setuid program drops the borrowed identity as soon as it's done.
 *
 * Build
 *   make
 *
 * Run
 *   Step 1, as a normal program:
 *     ./15_setuid_identity                     all three UIDs are you
 *   Step 2, look at a real setuid program:
 *     ls -l /usr/bin/passwd                    -rwsr-xr-x root root (see the s?)
 *   Step 3, make our program setuid root (needs sudo):
 *     sudo chown root:root 15_setuid_identity
 *     sudo chmod 4755 15_setuid_identity       the 4 in front is the setuid bit
 *     ls -l 15_setuid_identity                 now it shows rws too
 *     ./15_setuid_identity
 *   Step 4, clean up. Don't leave a setuid-root program lying around:
 *     sudo chmod u-s 15_setuid_identity        (make clean deletes the file too)
 *
 * What to look for
 *   Step 1: opening /etc/shadow fails with "Permission denied".
 *   Step 3: the real UID is still you, but the effective UID is 0 (root), and the
 *   open works. After setresuid() all three are you again and the open fails.
 *
 * Other ways to see it
 *   find /usr/bin -perm -4000          every setuid program in /usr/bin
 *   grep Uid /proc/self/status         real, effective, and saved UID, plus a fourth
 *                                      Linux-only one you can ignore
 *   Run passwd in one terminal and leave it sitting at the password prompt. Then in
 *   another terminal:
 *     ps -o pid,ruser,euser,cmd -C passwd
 *   RUSER (real) is you, EUSER (effective) is root. Ctrl-C the passwd afterwards.
 */
/* _GNU_SOURCE turns on getresuid() and setresuid(), which aren't in standard POSIX. */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 * Turn a UID number into a name by looking it up in /etc/passwd. "static" means this
 * function is only used in this file. getpwuid() returns NULL if there's no such user,
 * and a ? b : c picks the name if we found one, or "?" if not.
 */
static const char *user_name(uid_t uid)
{
    struct passwd *entry = getpwuid(uid);
    return entry != NULL ? entry->pw_name : "?";
}

/* Print all three UIDs, then try to open /etc/shadow to see what they let us do. */
static void show_identity(const char *moment)
{
    uid_t real_uid;
    uid_t effective_uid;
    uid_t saved_uid;
    int fd; /* a file descriptor: the small number the kernel gives you for an open file */

    /*
     * getresuid() fetches all three UIDs at once. The & gives it the address of each
     * variable so it can fill them in. It returns -1 on failure, and perror prints why.
     */
    if (getresuid(&real_uid, &effective_uid, &saved_uid) < 0) {
        perror("getresuid");
        return;
    }

    /* uid_t is the type for a user ID. The (long) casts make each one match %ld. */
    printf("%s\n", moment);
    printf("  real UID      = %ld (%s)\n", (long)real_uid, user_name(real_uid));
    printf("  effective UID = %ld (%s)\n", (long)effective_uid, user_name(effective_uid));
    printf("  saved UID     = %ld (%s)\n", (long)saved_uid, user_name(saved_uid));

    /*
     * /etc/shadow holds password hashes, and only root may read it. Whether this open
     * works depends on the EFFECTIVE UID, not the real one. O_RDONLY means read only.
     * open() returns -1 if it fails, and strerror(errno) turns the reason into words.
     * We close the file right away, since we only wanted to see if the open works.
     */
    fd = open("/etc/shadow", O_RDONLY);
    if (fd >= 0) {
        printf("  open(\"/etc/shadow\") succeeded: the kernel checked the effective UID\n");
        (void)close(fd);
    } else {
        printf("  open(\"/etc/shadow\") failed: %s\n", strerror(errno));
    }
}

int main(void)
{
    show_identity("At start:");

    /*
     * Same real and effective UID means the setuid bit isn't on, so there's nothing to
     * drop.
     * getuid() is the real UID and geteuid() is the effective one.
     */
    if (geteuid() == getuid()) {
        printf("\nReal and effective UIDs match: this copy is not setuid. See the Run steps above.\n");
        return 0;
    }

    /*
     * Drop back to the real user for good. Setting real, effective, AND saved to our
     * real UID means there's no copy of root left to switch back to.
     */
    if (setresuid(getuid(), getuid(), getuid()) < 0) {
        perror("setresuid");
        return 1;
    }
    show_identity("\nAfter setresuid(real, real, real) drops the borrowed identity:");
    return 0;
}
