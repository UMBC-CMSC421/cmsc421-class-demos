#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define MAX_DOC_LENGTH 256

char document[MAX_DOC_LENGTH]; // Simulated document text
pthread_mutex_t doc_mutex;     // Mutex to synchronize access to document

// Simulated user input function
void *user_input_thread(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&doc_mutex);
        printf("Type something: ");
        fgets(document, MAX_DOC_LENGTH, stdin);
        document[strcspn(document, "\n")] = 0; // Remove newline character
        pthread_mutex_unlock(&doc_mutex);
        sleep(1);
    }
    return NULL;
}

// Simulated spell check function
void *spell_check_thread(void *arg)
{
    while (1)
    {
        sleep(3); // Simulate time taken for spell check
        pthread_mutex_lock(&doc_mutex);
        printf("[Spell Check] Checking spelling in document: \"%s\"\n", document);
        pthread_mutex_unlock(&doc_mutex);
    }
    return NULL;
}

// Simulated autosave function
void *autosave_thread(void *arg)
{
    while (1)
    {
        sleep(5); // Save every 5 seconds
        pthread_mutex_lock(&doc_mutex);
        printf("[Autosave] Document \"%s\" saved!\n", document);
        pthread_mutex_unlock(&doc_mutex);
    }
    return NULL;
}

// Simulated print function
void *print_thread(void *arg)
{
    while (1)
    {
        sleep(10); // Print every 10 seconds
        pthread_mutex_lock(&doc_mutex);
        printf("[Printing] Sending document \"%s\" to printer...\n", document);
        pthread_mutex_unlock(&doc_mutex);
    }
    return NULL;
}

int main()
{
    pthread_t input_tid, spell_tid, autosave_tid, print_tid;

    // Initialize mutex
    pthread_mutex_init(&doc_mutex, NULL);

    // Create threads
    pthread_create(&input_tid, NULL, user_input_thread, NULL);
    pthread_create(&spell_tid, NULL, spell_check_thread, NULL);
    pthread_create(&autosave_tid, NULL, autosave_thread, NULL);
    pthread_create(&print_tid, NULL, print_thread, NULL);

    // Keep the main thread alive
    pthread_join(input_tid, NULL);
    pthread_join(spell_tid, NULL);
    pthread_join(autosave_tid, NULL);
    pthread_join(print_tid, NULL);

    // Destroy mutex
    pthread_mutex_destroy(&doc_mutex);

    return 0;
}
