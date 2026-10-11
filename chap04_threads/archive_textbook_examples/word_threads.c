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
    (void)arg; // unused
    char line[MAX_DOC_LENGTH]; // Private to this thread, so reading into it needs no lock

    while (1)
    {
        printf("Type something: ");
        fflush(stdout);

        // Wait for the user WITHOUT holding the mutex. Holding it across fgets() would
        // block spell check, autosave and printing until the user presses Enter.
        if (fgets(line, MAX_DOC_LENGTH, stdin) == NULL)
            break;                     // End of input (Ctrl-D)
        line[strcspn(line, "\n")] = 0; // Remove newline character

        // Hold the mutex only for the short copy into the shared document.
        pthread_mutex_lock(&doc_mutex);
        strcpy(document, line);
        pthread_mutex_unlock(&doc_mutex);
    }
    return NULL;
}

// Simulated spell check function
void *spell_check_thread(void *arg)
{
    (void)arg; // unused
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
    (void)arg; // unused
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
    (void)arg; // unused
    while (1)
    {
        sleep(10); // Print every 10 seconds
        pthread_mutex_lock(&doc_mutex);
        printf("[Printing] Sending document \"%s\" to printer...\n", document);
        pthread_mutex_unlock(&doc_mutex);
    }
    return NULL;
}

int main(void)
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
