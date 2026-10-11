/*
 * Chapter 4: Threads
 * Demo 06: Threaded word count
 *
 * What's going on
 *   This is data parallelism: split the input in two, give each half to a thread,
 *   and have each thread count the words in its half. Main then joins both threads
 *   and adds up the two partial counts.
 *
 *   The split point can't land in the middle of a word, or that word would be
 *   counted twice (once by each thread). So main starts at the halfway point and
 *   moves right until it reaches a space.
 *
 *   No mutex is needed. Both threads read the same text, but nobody writes to it,
 *   and each thread writes its count into its own struct. Nothing is written by
 *   one thread and touched by the other.
 *
 *   A "word" here is any run of characters with no whitespace in it.
 *
 * Build
 *   make
 *
 * Run
 *   ./06_word_count_threads
 *
 * What to look for
 *   [step 2]: the split moves right from byte 64 to the space at byte 65.
 *   Both workers print the same PID as main but different TIDs.
 *   The [worker 0] and [worker 1] lines can interleave with each other and with main.
 *   The result line says total=15 expected=15 CORRECT every run.
 */
#define _GNU_SOURCE   /* needed for gettid() */
#include <ctype.h>     /* isspace() */
#include <pthread.h>   /* pthread_t, pthread_create(), pthread_join() */
#include <stdio.h>     /* printf(), fprintf(), setbuf() */
#include <string.h>    /* strlen(), strerror() */
#include <sys/types.h> /* pid_t */
#include <unistd.h>    /* getpid(), gettid() */

/* One thread's piece of the work: which bytes to scan, and where to put the answer. */
struct job {
    int number;       /* worker number, for printing */
    const char *text; /* the whole input; the thread reads only its own range */
    size_t start;     /* first byte to scan */
    size_t end;       /* one past the last byte to scan */
    size_t count;     /* the thread writes its partial count here */
};

static void *count_chunk(void *arg)
{
    struct job *job = arg;
    size_t count = 0;
    int in_word = 0; /* 1 while we're in the middle of a word */

    printf("[worker %d] started: PID=%ld TID=%ld range=[%zu,%zu)\n",
           job->number, (long)getpid(), (long)gettid(), job->start, job->end);

    /* Count a word each time we step from whitespace onto a non-space character. */
    for (size_t i = job->start; i < job->end; i++) {
        /* isspace() needs a value that fits in unsigned char, so convert first. */
        unsigned char c = (unsigned char)job->text[i];

        if (isspace(c) != 0) {
            in_word = 0;
        } else if (!in_word) {
            count++;
            in_word = 1;
            printf("[worker %d] found word %zu at byte %zu\n",
                   job->number, count, i);
        }
    }

    /* Only this thread writes jobs[number].count, so no lock is needed. */
    job->count = count;
    printf("[worker %d] finished: partial count=%zu; storing it in jobs[%d].count\n",
           job->number, job->count, job->number);
    return NULL;
}

int main(void)
{
    const char *text =
        "Operating systems coordinate processes threads memory and devices "
        "while programmers reason about concurrency and synchronization.";
    enum { THREAD_COUNT = 2 };
    pthread_t threads[THREAD_COUNT];
    struct job jobs[THREAD_COUNT];
    size_t length = strlen(text);
    size_t midpoint = length / 2;
    size_t total = 0;
    int rc;

    /* Turn off output buffering so lines appear in the order they really happen. */
    setbuf(stdout, NULL);
    printf("=== Demo 06: threaded word count ===\n");
    printf("[step 1] main: PID=%ld TID=%ld\n",
           (long)getpid(), (long)gettid());
    printf("[step 1] input length=%zu bytes\n", length);
    printf("[step 1] input=\"%s\"\n", text);
    printf("[step 2] initial midpoint: %zu\n", midpoint);

    /* Move the split right until it sits on whitespace, so no word is cut in two. */
    while (midpoint < length && !isspace((unsigned char)text[midpoint])) {
        printf("[step 2] byte %zu is inside a word; moving the split right\n",
               midpoint);
        midpoint++;
    }
    printf("[step 2] safe whitespace boundary found at byte %zu\n", midpoint);

    /* Worker 0 gets the first half. */
    jobs[0].number = 0;
    jobs[0].text = text;
    jobs[0].start = 0;
    jobs[0].end = midpoint;
    jobs[0].count = 0;

    /* Worker 1 gets the rest. */
    jobs[1].number = 1;
    jobs[1].text = text;
    jobs[1].start = midpoint;
    jobs[1].end = length;
    jobs[1].count = 0;

    /* %.*s prints exactly that many characters, starting at the given address. */
    printf("[step 3] job 0 owns bytes [%zu,%zu): \"%.*s\"\n",
           jobs[0].start, jobs[0].end,
           (int)(jobs[0].end - jobs[0].start), jobs[0].text + jobs[0].start);
    printf("[step 3] job 1 owns bytes [%zu,%zu): \"%.*s\"\n",
           jobs[1].start, jobs[1].end,
           (int)(jobs[1].end - jobs[1].start), jobs[1].text + jobs[1].start);
    printf("[step 3] both workers read the same text, but write different count fields\n");

    /* Start both workers. Each gets the address of its own job struct. */
    for (int i = 0; i < THREAD_COUNT; i++) {
        printf("[step 4] main is creating worker %d with argument &jobs[%d]=%p\n",
               i, i, (void *)&jobs[i]);
        rc = pthread_create(&threads[i], NULL, count_chunk, &jobs[i]);
        if (rc != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(rc));
            return 1;
        }
        printf("[step 4] pthread_create for worker %d returned success\n", i);
    }

    /*
     * Join each worker, then read its count. The join comes first: until it returns,
     * the worker may still be writing jobs[i].count.
     */
    for (int i = 0; i < THREAD_COUNT; i++) {
        printf("[step 5] main is joining worker %d\n", i);
        rc = pthread_join(threads[i], NULL);
        if (rc != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(rc));
            return 1;
        }
        total += jobs[i].count;
        printf("[step 5] worker %d joined; partial=%zu; running total=%zu\n",
               i, jobs[i].count, total);
    }

    printf("[result] total=%zu expected=15 %s\n",
           total, total == 15 ? "CORRECT" : "WRONG");
    printf("[why no mutex] text was read-only and each worker owned a separate jobs[i].count\n");
    return 0;
}
