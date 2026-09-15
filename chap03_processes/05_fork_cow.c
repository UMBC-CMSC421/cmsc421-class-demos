/*
 * Chapter 3: Processes
 * Demo 05: After fork(), parent and child have separate memory
 *
 * What's going on
 *   The child starts with a copy of every variable the parent had: same names, same
 *   values, even the same addresses. But they're separate copies. When the child
 *   changes its counters, the parent's counters don't move.
 *
 *   About the name "copy-on-write" (COW): Linux doesn't actually copy all of the
 *   memory at fork() time, because that would be slow. At first parent and child
 *   share the same physical pages, marked read-only. When one of them writes to a
 *   page, the kernel quietly makes a private copy of just that page. The program
 *   can't see that happen. What it can see is the result: the values end up
 *   separate, and that's what this demo prints.
 *
 * Build
 *   make
 *
 * Run
 *   ./05_fork_cow
 *
 * What to look for
 *   The child prints 101 and 201. The parent, which prints after the child is done,
 *   still has 100 and 200. The addresses are identical in both. Those are virtual
 *   addresses. Each process has its own mapping from virtual addresses to real RAM,
 *   so the same address can point at different memory in different processes.
 *
 * Other ways to see it
 *   This one is over in a millisecond, so outside tools don't catch much. Try
 *   editing it instead, then run make again:
 *   - Add a printf in the child BEFORE the += 1 lines. It shows 100 and 200, so the
 *     child really did start with the parent's values.
 *   - Move the two += 1 lines out of the child and into the parent, after waitpid().
 *     Now the parent's numbers change and the child's don't.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int global_counter = 100; /* a global, in the data area */

int main(void)
{
    int local_counter = 200; /* a local, on the stack */
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        /* Child: change our copies. This is the "write" in copy-on-write. */
        global_counter += 1;
        local_counter += 1;

        printf("[child ] global_counter=%d at %p\n", global_counter, (void *)&global_counter);
        printf("[child ] local_counter =%d at %p\n", local_counter, (void *)&local_counter);
        fflush(stdout);
        _exit(0); /* _exit instead of exit: see the comment in demo 04 */
    }

    /* Parent: wait until the child has changed its values and exited, then print ours. */
    if (waitpid(pid, NULL, 0) < 0) {
        perror("waitpid");
        return 1;
    }

    printf("[parent] global_counter=%d at %p\n", global_counter, (void *)&global_counter);
    printf("[parent] local_counter =%d at %p\n", local_counter, (void *)&local_counter);

    return 0;
}
