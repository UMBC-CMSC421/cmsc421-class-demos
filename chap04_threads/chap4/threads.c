// thread_spawn_demo.c
// Focus: create N threads, name them on macOS, print their thread IDs.
// How-Build: cc -Wall -Wextra -pthread -o thread_demo thread_spawn_demo.c

#define _DARWIN_C_SOURCE 1 // Enable macOS-specific APIs (e.g., pthread_threadid_np, pthread_setname_np).
#include <pthread.h>       // POSIX threads API: pthread_t, pthread_create, pthread_join, etc.
#include <stdio.h>         // printf, fprintf, perror, puts
#include <stdlib.h>        // atoi, calloc, free
#include <stdint.h>        // uint64_t
#include <unistd.h>        // sleep

// A small per-thread argument bundle. This is just the thread "index".
typedef struct
{
    int index; // Which worker number this is (0..N-1)
} thread_arg_t;

// Helper function to print a label and the current thread's ID in a stable way on macOS.
static void print_thread_id(const char *label)
{
    uint64_t tid = 0; // We'll store a numeric thread ID here.

    // macOS-specific; returns a stable numeric thread id for teaching purposes.
    // Passing NULL means "the current thread".
    if (pthread_threadid_np(NULL, &tid) == 0)
    {
        // %llu because tid is 64-bit; cast to unsigned long long for printf portability.
        printf("%s: pthread_threadid_np = %llu\n", label, (unsigned long long)tid);
    }
    else
    {
        // Fallback: show pthread_self(). It's opaque (not a real numeric ID),
        // but okay for demo purposes when the macOS call fails or on other platforms.
        printf("%s: pthread_self = %p\n", label, (void *)pthread_self());
    }
}

// The function each worker thread will run.
// It must have signature void* (*)(void*).
void *worker(void *arg)
{
    // Recover our typed pointer from the generic void*.
    thread_arg_t *a = (thread_arg_t *)arg;

    // On macOS, the only thread you can name is the *current* one.
    // We'll build a name like "worker-3".
    char name[32];
    snprintf(name, sizeof(name), "worker-%d", a->index);

    // Name this thread (macOS-specific). On success, debuggers and tools show this name.
    pthread_setname_np(name);

    // Print a label and this thread's ID so students can correlate names to IDs.
    print_thread_id(name);

    // Keep the thread alive briefly so tools (ps/LLDB/Activity Monitor) can see it.
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
