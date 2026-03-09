/*
 * Chapter 6: Synchronization Tools
 * Demo 03: Producer-consumer with semaphores
 *
 * What this demo shows:
 * - empty_slots counts available space
 * - full_slots counts available items
 * - a mutex protects the shared buffer indices
 */
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

enum {
    BUFFER_SIZE = 5,
    ITEM_COUNT = 10
};

static int buffer[BUFFER_SIZE];
static int in_index = 0;
static int out_index = 0;

static sem_t empty_slots;
static sem_t full_slots;
static pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;

static void *producer(void *arg)
{
    (void)arg;

    for (int item = 1; item <= ITEM_COUNT; item++) {
        sem_wait(&empty_slots);
        pthread_mutex_lock(&buffer_lock);

        buffer[in_index] = item;
        printf("[producer] produced %d in slot %d\n", item, in_index);
        in_index = (in_index + 1) % BUFFER_SIZE;

        pthread_mutex_unlock(&buffer_lock);
        sem_post(&full_slots);
    }

    return NULL;
}

static void *consumer(void *arg)
{
    (void)arg;

    for (int i = 0; i < ITEM_COUNT; i++) {
        int item;

        sem_wait(&full_slots);
        pthread_mutex_lock(&buffer_lock);

        item = buffer[out_index];
        printf("[consumer] consumed %d from slot %d\n", item, out_index);
        out_index = (out_index + 1) % BUFFER_SIZE;

        pthread_mutex_unlock(&buffer_lock);
        sem_post(&empty_slots);
    }

    return NULL;
}

int main(void)
{
    pthread_t producer_thread;
    pthread_t consumer_thread;

    if (sem_init(&empty_slots, 0, BUFFER_SIZE) != 0) {
        perror("sem_init empty_slots");
        return 1;
    }

    if (sem_init(&full_slots, 0, 0) != 0) {
        perror("sem_init full_slots");
        return 1;
    }

    if (pthread_create(&producer_thread, NULL, producer, NULL) != 0) {
        perror("pthread_create producer");
        return 1;
    }

    if (pthread_create(&consumer_thread, NULL, consumer, NULL) != 0) {
        perror("pthread_create consumer");
        return 1;
    }

    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    sem_destroy(&empty_slots);
    sem_destroy(&full_slots);
    pthread_mutex_destroy(&buffer_lock);

    return 0;
}
