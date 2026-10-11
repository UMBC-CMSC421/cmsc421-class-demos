// threads.c
// Focus: create N threads, name them, print their thread IDs.
// How-Build: make archive (or: cc -Wall -Wextra -pthread -o threads threads.c)
// See the named threads while the workers sleep:
//   ./threads & ps -L -o pid,tid,comm -p $!

#define _GNU_SOURCE        // Enable Linux-specific APIs (gettid, pthread_setname_np).
#include <pthread.h>       // POSIX threads API: pthread_t, pthread_create, pthread_join, etc.
#include <stdio.h>         // printf, fprintf, perror, puts
#include <stdlib.h>        // atoi, calloc, free
#include <unistd.h>        // sleep, getpid, gettid

// A small per-thread argument bundle. This is just the thread "index".
typedef struct
{
    int index; // Which worker number this is (0..N-1)
} thread_arg_t;

// Print a label, the process ID, and the current thread's ID.
static void print_thread_id(const char *label)
{
    // Every thread has a kernel thread ID (TID); all threads share the process's PID.
    // These are the same numbers ps -L and /proc/<PID>/task show.
    printf("%s: PID %ld  TID %ld\n", label, (long)getpid(), (long)gettid());
}

// The function each worker thread will run.
// It must have signature void* (*)(void*).
void *worker(void *arg)
{
    // Recover our typed pointer from the generic void*.
    thread_arg_t *a = (thread_arg_t *)arg;

    // We'll build a name like "worker-3". Linux limits thread names to 15 characters.
    char name[32];
    snprintf(name, sizeof(name), "worker-%d", a->index);

    // Name this thread. ps -L -o comm and gdb show this name.
    // The _np suffix means non-portable: this call is a Linux extension, not standard POSIX.
    pthread_setname_np(pthread_self(), name);

    // Print a label and this thread's ID so students can correlate names to IDs.
    print_thread_id(name);

    // Keep the thread alive briefly so tools (ps, top -H, gdb) can see it.
    sleep(5);

    // Returning NULL is equivalent to calling pthread_exit(NULL).
    return NULL;
}

// Entry point: parse N, start N threads, then join them.
int main(int argc, char **argv)
{
    // If argv[1] exists, parse it as the number of threads; else default to 4.
    int n = (argc > 1) ? atoi(argv[1]) : 4;

    // Clamp to a reasonable range so we don't go wild on a student's laptop.
    if (n < 1)
        n = 1;
    if (n > 64)
        n = 64;

    // Let the user know what's about to happen, and print the main thread's ID.
    printf("Starting thread demo (creating %d threads)\n", n);
    print_thread_id("main");

    // Allocate arrays:
    //  - 'threads' holds pthread_t handles returned by pthread_create.
    //  - 'args' holds per-thread argument structs so each thread gets a stable address.
    pthread_t *threads = calloc((size_t)n, sizeof(pthread_t));
    thread_arg_t *args = calloc((size_t)n, sizeof(thread_arg_t));

    // Always check allocations; if either failed, report and exit nonzero.
    if (!threads || !args)
    {
        perror("calloc");
        return 1;
    }

    // Create N threads.
    for (int i = 0; i < n; i++)
    {
        // Store the index for this thread in a dedicated slot.
        // IMPORTANT: we pass &args[i] (a unique, stable address), not &i.
        args[i].index = i;

        // Start the thread; it begins executing worker(&args[i]).
        // The second parameter (NULL) means "use default thread attributes".
        int rc = pthread_create(&threads[i], NULL, worker, &args[i]);
        if (rc != 0)
        {
            // If thread creation fails at some i, report the error and stop creating more.
            fprintf(stderr, "pthread_create failed at i=%d (rc=%d)\n", i, rc);
            n = i; // Only join the threads that actually started (0..i-1).
            break;
        }
    }

    // Join all successfully created threads so we wait for them to finish.
    // This prevents the process from exiting while workers are still running.
    for (int i = 0; i < n; i++)
    {
        // Wait for threads[i] to terminate; we ignore its return value here (NULL).
        pthread_join(threads[i], NULL);
    }

    // Free the arrays we allocated.
    free(threads);
    free(args);

    // Final message to show we've joined everything and are shutting down cleanly.
    puts("All threads joined. Exiting.");

    // Return 0 (success).
    return 0;
}
