/*
 * Chapter 4: Threads
 * Demo 06: Threaded word counting by chunk
 *
 * What this demo shows:
 * - Split work among threads.
 * - Each thread computes a partial result.
 * - The main thread joins workers and combines results.
 *
 * For simplicity, this demo treats a "word" as a maximal run of non-space bytes.
 */
#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct job {
    const char *text;
    size_t start;
    size_t end;
    size_t count;
};

static void *count_chunk(void *arg)
{
    struct job *job = arg;
    size_t count = 0;
    int in_word = 0;

    for (size_t i = job->start; i < job->end; i++) {
        unsigned char c = (unsigned char)job->text[i];

        if (isspace(c) != 0) {
            in_word = 0;
        } else if (!in_word) {
            count++;
            in_word = 1;
        }
    }

    job->count = count;
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

    while (midpoint < length && !isspace((unsigned char)text[midpoint])) {
        midpoint++;
    }

    jobs[0].text = text;
    jobs[0].start = 0;
    jobs[0].end = midpoint;
    jobs[0].count = 0;

    jobs[1].text = text;
    jobs[1].start = midpoint;
    jobs[1].end = length;
    jobs[1].count = 0;

    for (int i = 0; i < THREAD_COUNT; i++) {
        if (pthread_create(&threads[i], NULL, count_chunk, &jobs[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    for (int i = 0; i < THREAD_COUNT; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            return 1;
        }
        total += jobs[i].count;
    }

    printf("Text: %s\n", text);
    printf("Estimated word count = %zu\n", total);
    return 0;
}
