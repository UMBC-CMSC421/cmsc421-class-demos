// demo2_mutex_vs_semaphore.c
// Shows: (A) mutex fixes counter++ race, (B) semaphore limits concurrency.
//
// Build: gcc -O2 -pthread demo2_mutex_vs_semaphore.c -o demo2
// ./demo2 8 5000000 3

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

static int64_t counter = 0;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

// For part B: allow only K threads in the "resource" at once
static sem_t sem;
static int inside = 0;
static pthread_mutex_t inside_mtx = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int64_t iters;
} args_t;

static void* mutex_counter_worker(void* arg) {
    args_t* a = (args_t*)arg;
    for (int64_t i = 0; i < a->iters; i++) {
        pthread_mutex_lock(&mtx);
        counter++;   // now protected
        pthread_mutex_unlock(&mtx);
    }
    return NULL;
}

typedef struct {
    int id;
    unsigned int work_seconds;
} sem_args_t;

static void* semaphore_resource_worker(void* arg) {
    sem_args_t* a = (sem_args_t*)arg;

    // Wait for a permit (may block)
    sem_wait(&sem);

    // Enter "resource"
    pthread_mutex_lock(&inside_mtx);
    inside++;
    printf("[T%d] entered resource; inside=%d\n", a->id, inside);
    pthread_mutex_unlock(&inside_mtx);

    // Simulate work holding the resource
    sleep(a->work_seconds);
    printf("[T%d] slept for %d seconds; inside=%d\n", a->id, a->work_seconds, inside);

    // Exit "resource"
    pthread_mutex_lock(&inside_mtx);
    inside--;
    printf("[T%d] leaving resource; inside=%d\n", a->id, inside);
    pthread_mutex_unlock(&inside_mtx);

    // Release permit
    sem_post(&sem);
    return NULL;
}

int main(int argc, char** argv) {
    int threads = 8;
    int64_t iters = 5000000;
    int permits = 3;

    if (argc >= 2) threads = atoi(argv[1]);
    if (argc >= 3) iters = atoll(argv[2]);
    if (argc >= 4) permits = atoi(argv[3]);

    if (threads <= 0 || iters <= 0 || permits <= 0) {
        fprintf(stderr, "Usage: %s [threads>=1] [iters>=1] [permits>=1]\n", argv[0]);
        return 1;
    }

    // ---------- Part A: mutex correctness ----------
    pthread_t* tids = calloc((size_t)threads, sizeof(pthread_t));
    if (!tids) { perror("calloc"); return 1; }

    args_t a = {.iters = iters};
    counter = 0;

    for (int i = 0; i < threads; i++) {
        pthread_create(&tids[i], NULL, mutex_counter_worker, &a);
    }
    for (int i = 0; i < threads; i++) {
        pthread_join(tids[i], NULL);
    }

    int64_t expected = (int64_t)threads * iters;
    printf("\n[Part A] Mutex-protected counter\n");
    printf("Expected: %lld\n", (long long)expected);
    printf("Actual:   %lld\n", (long long)counter);

    // ---------- Part B: semaphore correctness ----------
    printf("\n[Part B] Semaphore-limited resource (permits=%d)\n", permits);
    sem_init(&sem, 0, (unsigned)permits);

    pthread_t* tids2 = calloc((size_t)threads, sizeof(pthread_t));
    sem_args_t* args2 = calloc((size_t)threads, sizeof(sem_args_t));
    if (!tids2 || !args2) { perror("calloc"); return 1; }

    for (int i = 0; i < threads; i++) {
        args2[i].id = i;
        args2[i].work_seconds = 2; // hold resource for 2s 
        pthread_create(&tids2[i], NULL, semaphore_resource_worker, &args2[i]);
    }
    for (int i = 0; i < threads; i++) {
        pthread_join(tids2[i], NULL);
    }

    sem_destroy(&sem);
    free(tids);
    free(tids2);
    free(args2);
    return 0;
}
