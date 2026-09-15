/*
 * Chapter 4: Threads
 * Demo 07: Threads versus processes
 *
 * What this demo shows:
 * - Threads of one process share its PID and its memory; each thread has its own
 *   thread ID (TID) (Fox 4.2.2 and 4.5). The first thread's TID equals the PID.
 * - A change made by one thread is seen by the others. A forked child is a separate
 *   process with its own copy of memory, so its change is not.
 * - fork() in a multithreaded program copies only the calling thread.
 * - Linux lists a process's threads in /proc/<PID>/task, and ps shows them with -L.
 *
 * Run:
 *   ./07_threads_vs_processes
 * While it waits for Enter, in another terminal:
 *   ps -L -o pid,tid,nlwp,stat,cmd -p <PID>
 *   ls /proc/<PID>/task
 */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define THREADS 3

static int shared_counter = 0;
static int finished = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t finish = PTHREAD_COND_INITIALIZER;

static int thread_count(void)
{
    char line[128];
    int count = -1;
    FILE *status = fopen("/proc/self/status", "r");

    if (status == NULL) {
        return -1;
    }
    while (fgets(line, sizeof(line), status) != NULL) {
        (void)sscanf(line, "Threads: %d", &count);
    }
    (void)fclose(status);
    return count;
}

static void *worker(void *arg)
{
    int number = *(const int *)arg;

    pthread_mutex_lock(&lock);
    shared_counter += 10;
    printf("  thread %d: PID %ld  TID %ld  shared_counter = %d\n",
           number, (long)getpid(), (long)gettid(), shared_counter);
    fflush(stdout);
    while (!finished) {
        pthread_cond_wait(&finish, &lock);
    }
    pthread_mutex_unlock(&lock);
    return NULL;
}

int main(void)
{
    pthread_t threads[THREADS];
    int numbers[THREADS];
    struct timespec settle = {.tv_sec = 0, .tv_nsec = 200L * 1000L * 1000L};
    pid_t child;

    printf("main thread: PID %ld  TID %ld  (the first thread's TID is the PID)\n",
           (long)getpid(), (long)gettid());

    for (int i = 0; i < THREADS; i++) {
        numbers[i] = i + 1;
        if (pthread_create(&threads[i], NULL, worker, &numbers[i]) != 0) {
            fprintf(stderr, "pthread_create failed\n");
            return 1;
        }
    }
    (void)nanosleep(&settle, NULL);

    pthread_mutex_lock(&lock);
    printf("main thread: shared_counter = %d, threads in this process = %d\n\n",
           shared_counter, thread_count());
    pthread_mutex_unlock(&lock);
    fflush(stdout);

    child = fork();
    if (child < 0) {
        perror("fork");
        return 1;
    }
    if (child == 0) {
        shared_counter += 1000;
        printf("  child process: PID %ld  TID %ld  PPID %ld  shared_counter = %d  threads = %d\n",
               (long)getpid(), (long)gettid(), (long)getppid(), shared_counter, thread_count());
        fflush(stdout);
        _exit(0);
    }
    (void)waitpid(child, NULL, 0);

    printf("\nmain thread after the child exited: shared_counter is still %d\n", shared_counter);
    printf("\nIn another terminal:\n  ps -L -o pid,tid,nlwp,stat,cmd -p %ld\n  ls /proc/%ld/task\n",
           (long)getpid(), (long)getpid());
    printf("Press Enter to finish.\n");
    fflush(stdout);
    (void)getchar();

    pthread_mutex_lock(&lock);
    finished = 1;
    pthread_cond_broadcast(&finish);
    pthread_mutex_unlock(&lock);
    for (int i = 0; i < THREADS; i++) {
        (void)pthread_join(threads[i], NULL);
    }
    return 0;
}
