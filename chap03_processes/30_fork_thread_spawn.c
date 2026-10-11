/*
 * Chapter 3: Processes
 * Demo 30: fork, pthread_create, and posix_spawn all reach the kernel through clone
 *
 * What's going on
 *   The textbook teaches fork() as "the" way to create a process. On Linux today,
 *   fork(), pthread_create(), and posix_spawn() are C library functions, and all
 *   three end up in the same kernel call family: clone() or clone3(). What differs
 *   is the flags, which tell the kernel what the new task shares with its creator:
 *     fork()          clone(SIGCHLD ...)               shares nothing: a new process
 *     pthread_create  clone3(CLONE_VM|CLONE_FILES|      shares memory, files, signal
 *                            CLONE_SIGHAND|CLONE_THREAD) handlers: a new thread
 *     posix_spawn     clone3(CLONE_VM|CLONE_VFORK ...)  borrows the parent's memory
 *                                                       until exec, so nothing is copied
 *   (glibc 2.34 and later use clone3 for threads, including glibc 2.36 on the course
 *   VM; posix_spawn has used the CLONE_VM|CLONE_VFORK trick since glibc 2.24.)
 *
 *   So to the kernel, a process and a thread are both just "tasks." The flags
 *   decide how much a new task shares with the one that made it.
 *
 * Build
 *   make 30_fork_thread_spawn      (link needs -pthread; the Makefile adds it)
 *
 * Run
 *   strace -f -e trace=clone,clone3,execve ./30_fork_thread_spawn
 *
 * What to look for
 *   Three clone/clone3 lines, one per creation call, and the flags on each.
 *   The thread's line has CLONE_THREAD; the spawn line has CLONE_VFORK and is
 *   followed by an execve of /bin/echo in the child.
 *   Without strace the program only prints three short lines.
 *
 * Other ways to see it
 *   man 2 clone        the flag list
 *   man 3 posix_spawn  the spawn interface
 */
#include <pthread.h>
#include <spawn.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* environ is the C library's list of this process's environment variables
   (PATH, HOME, and so on). We pass it to posix_spawn so echo gets the same ones. */
extern char **environ;

/* The new thread starts running here. pthread_create needs a function that takes
   a void * and returns a void *, even though we don't use either one. */
static void *thread_body(void *arg)
{
    (void)arg; /* (void) marks arg as unused on purpose, so the compiler won't warn */
    /* getpid() returns a pid_t. The (int) cast makes it match the %d format. */
    printf("  thread: same PID %d, shares the process's memory\n", (int)getpid());
    return NULL;
}

int main(void)
{
    pid_t pid;          /* pid_t is the type the system uses for process IDs */
    pthread_t tid;      /* a handle for the thread, filled in by pthread_create */
    /* The argument list for the spawned program, like argv in main. It must end
       with NULL so echo knows where the list stops. */
    char *argv[] = {"echo", "  spawned: echo ran in a new process", NULL};

    fflush(stdout); /* empty stdout's buffer before fork so nothing prints twice */

    /* 1. fork: a new process with its own (copy-on-write) memory. */
    pid = fork();
    if (pid < 0) {
        perror("fork"); /* perror prints your text plus the reason the call failed */
        return 1;
    }
    if (pid == 0) {
        /* Child: print its own PID and leave. _exit() skips the cleanup code the
           child inherited from the parent; fflush first so the line isn't lost. */
        printf("  fork child: PID %d\n", (int)getpid());
        fflush(stdout);
        _exit(0);
    }
    /* Parent: wait for the child. (void) means we're ignoring the return value
       on purpose. */
    (void)waitpid(pid, NULL, 0);

    /* 2. pthread_create: a new thread inside this same process.
       Unlike most calls, it returns 0 on success and an error number on failure. */
    if (pthread_create(&tid, NULL, thread_body, NULL) != 0) {
        fprintf(stderr, "pthread_create failed\n");
        return 1;
    }
    (void)pthread_join(tid, NULL); /* wait for the thread to finish, like waitpid */
    fflush(stdout);

    /* 3. posix_spawn: create and load a program in one call, no fork copy.
       It writes the new child's PID into pid. The two NULLs mean "no extra
       settings." Like pthread_create, it returns 0 on success. */
    if (posix_spawn(&pid, "/bin/echo", NULL, NULL, argv, environ) != 0) {
        fprintf(stderr, "posix_spawn failed\n");
        return 1;
    }
    (void)waitpid(pid, NULL, 0);
    return 0;
}
