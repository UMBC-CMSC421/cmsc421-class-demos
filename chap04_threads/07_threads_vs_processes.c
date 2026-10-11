/*
 * Chapter 4: Threads
 * Demo 07: Threads versus processes
 *
 * What's going on
 *   Threads of one process share its code and data (Fox 4.2.2 and 4.5). On Linux they
 *   also share one PID, and each thread has its own thread ID (TID). The first
 *   thread's TID is the same number as the PID.
 *
 *   Three worker threads each add 10 to one global, shared_counter, and main sees all
 *   three changes. Then main forks a child. The child is a new process with its own
 *   copy of memory, so when it adds 1000 to its shared_counter, the parent's doesn't
 *   change.
 *
 *   fork() in a program with several threads copies only the thread that called
 *   fork(). The other threads don't exist in the child. If one of them held a lock
 *   (stdio has locks inside printf) at that instant, the lock stays held in the child
 *   forever. So the child here only calls functions that take no locks, like write()
 *   and _exit(), and reports its counter to the parent through a pipe.
 *
 *   To keep the workers alive long enough to look at, each one sleeps on a condition
 *   variable until main tells it to finish. A condition variable lets a thread sleep
 *   until another thread changes something and wakes it.
 *
 * Build
 *   make
 *
 * Run
 *   ./07_threads_vs_processes
 *   It stops and waits for Enter so you can look at the threads.
 *
 * What to look for
 *   Every [worker] line shows the same PID as main, with a different TID.
 *   shared_counter goes 0, 10, 20, 30, and main sees 30.
 *   The child reports 1030, but main's shared_counter is still 30 afterward.
 *
 * Other ways to see it (Fox 4.4), in a second terminal while it waits for Enter
 *   ps -L -o pid,tid,nlwp,stat,cmd -p <PID>
 *     4 rows, one per thread: same PID, four TIDs, NLWP (number of threads) 4.
 *   ls /proc/<PID>/task
 *     one directory per thread, named by TID.
 */
#define _GNU_SOURCE     /* needed for gettid() */
#include <errno.h>      /* errno, EINTR */
#include <pthread.h>    /* threads, mutexes, condition variables */
#include <stdio.h>      /* printf(), fprintf(), perror(), fopen(), fgets(), sscanf() */
#include <string.h>     /* strerror() */
#include <sys/types.h>  /* pid_t, ssize_t */
#include <sys/wait.h>   /* waitpid() */
#include <unistd.h>     /* fork(), pipe(), read(), write(), close(), getpid(), gettid() */

#define THREADS 3

static int shared_counter = 0;
static int started = 0;  /* how many workers have printed their line */
static int finished = 0; /* set by main when the workers may exit */

/*
 * The lock protects the three variables above. A thread that has to wait for one of them
 * to change sleeps on the condition variable. Whoever changes one wakes the sleepers.
 */
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER;

/*
 * Send one int down a pipe. The child calls this after fork(), so it uses write(), which
 * takes no locks, instead of stdio. write() can send fewer bytes than asked for, so loop
 * until every byte of the int is sent. EINTR means a signal interrupted the call: try again.
 */
static int write_int(int fd, int value)
{
    const unsigned char *bytes = (const unsigned char *)&value; /* the int's raw bytes */
    size_t written = 0;

    while (written < sizeof(value)) {
        ssize_t amount = write(fd, bytes + written, sizeof(value) - written);

        if (amount < 0 && errno == EINTR) {
            continue;
        }
        if (amount <= 0) {
            return -1;
        }
        written += (size_t)amount;
    }
    return 0;
}

/* Read one int from a pipe, looping the same way write_int() does. */
static int read_int(int fd, int *value)
{
    unsigned char *bytes = (unsigned char *)value;
    size_t received = 0;

    while (received < sizeof(*value)) {
        ssize_t amount = read(fd, bytes + received, sizeof(*value) - received);

        if (amount < 0 && errno == EINTR) {
            continue;
        }
        if (amount <= 0) { /* 0 means the writer closed the pipe before sending it all */
            return -1;
        }
        received += (size_t)amount;
    }
    return 0;
}

/* Read this process's thread count from the "Threads:" line of /proc/self/status. */
static int thread_count(void)
{
    char line[128];
    int count = -1; /* -1 means "not found" */
    FILE *status = fopen("/proc/self/status", "r");

    if (status == NULL) {
        return -1;
    }
    while (fgets(line, sizeof(line), status) != NULL) {
        (void)sscanf(line, "Threads: %d", &count); /* fills count only on the matching line */
    }
    (void)fclose(status);
    return count;
}

static void *worker(void *arg)
{
    int number = *(const int *)arg; /* arg points at this worker's numbers[i] */
    int before;

    /* Everything from here to the unlock happens while this thread holds the lock. */
    pthread_mutex_lock(&lock);
    before = shared_counter;
    shared_counter += 10;
    printf("[worker %d] started: PID=%ld TID=%ld argument=%p\n",
           number, (long)getpid(), (long)gettid(), arg);
    printf("[worker %d] owns the lock: shared_counter %d -> %d\n",
           number, before, shared_counter);
    fflush(stdout);
    started++;
    printf("[worker %d] started count is now %d; waking main if it is waiting\n",
           number, started);
    pthread_cond_broadcast(&changed); /* wake every thread sleeping on changed */

    /*
     * Stay alive, asleep, until main says to finish, so ps can show this thread.
     * pthread_cond_wait() unlocks the lock while it sleeps and locks it again before it
     * returns. The while loop checks finished again after every wakeup, because a
     * wakeup only means something changed, not that finished is now 1.
     */
    printf("[worker %d] sleeping on the condition variable so ps can observe it\n",
           number);
    while (!finished) {
        pthread_cond_wait(&changed, &lock);
    }
    printf("[worker %d] observed finished=1 and is returning\n", number);
    pthread_mutex_unlock(&lock);
    return NULL;
}

int main(void)
{
    pthread_t threads[THREADS];
    int numbers[THREADS];     /* each worker's argument; stays valid until the joins */
    int report_pipe[2];       /* [0] is the read end, [1] is the write end */
    int child_counter = -1;
    int child_status;
    pid_t child;
    int rc;

    /* Turn off output buffering so lines appear in the order they really happen. */
    setbuf(stdout, NULL);
    printf("=== Demo 07: threads share; processes copy ===\n");
    printf("[step 1] main thread: PID=%ld TID=%ld\n",
           (long)getpid(), (long)gettid());
    printf("[step 1] the initial thread's Linux TID equals the process PID\n");
    printf("[step 1] initial shared_counter=%d\n", shared_counter);
    printf("[step 2] creating %d workers before joining any of them\n", THREADS);

    for (int i = 0; i < THREADS; i++) {
        numbers[i] = i + 1;
        printf("[step 2] pthread_create worker %d with stable argument &numbers[%d]=%p\n",
               numbers[i], i, (void *)&numbers[i]);
        rc = pthread_create(&threads[i], NULL, worker, &numbers[i]);
        if (rc != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(rc));
            return 1;
        }
        printf("[step 2] worker %d was created successfully\n", numbers[i]);
    }

    /* Wait until all the workers have started and printed their lines. */
    printf("[step 3] main locks shared state and waits until started reaches %d\n",
           THREADS);
    pthread_mutex_lock(&lock);
    while (started < THREADS) {
        printf("[step 3] started=%d; condition wait releases the lock while main sleeps\n",
               started);
        pthread_cond_wait(&changed, &lock);
    }
    printf("[step 3] all workers reported: shared_counter=%d process thread count=%d\n",
           shared_counter, thread_count());
    pthread_mutex_unlock(&lock);
    printf("[step 3] main unlocked the shared state\n");

    /* The pipe is how the child sends its counter back to the parent. */
    printf("[step 4] pipe() creates a read end [0] and write end [1]\n");
    if (pipe(report_pipe) != 0) {
        perror("pipe");
        return 1;
    }

    printf("[step 4] fork() now creates a separate child process\n");
    child = fork();
    if (child < 0) {
        perror("fork");
        (void)close(report_pipe[0]);
        (void)close(report_pipe[1]);
        return 1;
    }
    if (child == 0) {
        /*
         * Child: only one thread, a copy of main. No printf here (see the top of the
         * file). Change the child's own copy of the counter and send it to the parent.
         */
        (void)close(report_pipe[0]);
        shared_counter += 1000;
        child_status = write_int(report_pipe[1], shared_counter) == 0 ? 0 : 1;
        (void)close(report_pipe[1]);
        _exit(child_status); /* _exit(), not exit(): skip flushing stdio in the child */
    }

    /* Parent: close the write end it doesn't use, then read the child's report. */
    printf("[step 4] parent: fork returned child PID=%ld\n", (long)child);
    (void)close(report_pipe[1]);
    printf("[step 4] parent closed the pipe's write end and reads the child's report\n");
    if (read_int(report_pipe[0], &child_counter) != 0) {
        fprintf(stderr, "could not read the child's counter report\n");
    }
    (void)close(report_pipe[0]);
    printf("[step 4] child reported its private shared_counter=%d\n", child_counter);
    printf("[step 4] parent calls waitpid(%ld, ...) for child termination\n",
           (long)child);
    if (waitpid(child, &child_status, 0) < 0) {
        perror("waitpid");
        return 1;
    }

    /* The child had one thread, so its only TID was its PID. */
    printf("[child summary] PID=%ld had one thread (TID=%ld) and its own shared_counter=%d\n",
           (long)child, (long)child, child_counter);

    printf("[step 5] main thread after the child exited: shared_counter is still %d\n",
           shared_counter);
    printf("[step 5] the child's write was private; the workers' writes remain visible\n");
    printf("\n[observe] in another terminal:\n  ps -L -o pid,tid,nlwp,stat,cmd -p %ld\n  ls /proc/%ld/task\n",
           (long)getpid(), (long)getpid());
    printf("[observe] press Enter after inspecting the live workers.\n");
    (void)getchar();

    /* Tell the workers to finish, wake them all, then wait for each one. */
    printf("[step 6] main sets finished=1 and broadcasts to all sleeping workers\n");
    pthread_mutex_lock(&lock);
    finished = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&lock);
    for (int i = 0; i < THREADS; i++) {
        printf("[step 6] main joins worker %d\n", numbers[i]);
        (void)pthread_join(threads[i], NULL);
        printf("[step 6] worker %d joined successfully\n", numbers[i]);
    }
    printf("[result] all workers terminated; the process can exit safely\n");
    return 0;
}
