/*
 * Chapter 3: Processes
 * Demo 02: Memory layout
 *
 * What this demo shows:
 * - A process has logically distinct memory regions.
 * - Global/static objects live separately from stack and heap objects.
 * - The exact addresses are OS- and build-dependent.
 *
 * Suggested discussion:
 * - Which variables are in static storage?
 * - Which variables are on the stack?
 * - Which objects come from the heap?
 */
#include <stdio.h>
#include <stdlib.h>

static int static_global = 42;
int initialized_global = 7;
int uninitialized_global;

static void sample_function(void)
{
    /* Empty on purpose: we want its address. */
}

int main(void)
{
    int local = 99;
    static int static_local = 123;
    int *heap_value = malloc(sizeof(*heap_value));

    if (heap_value == NULL) {
        perror("malloc");
        return 1;
    }

    *heap_value = 555;

    printf("Address of sample_function     : %p\n", (void *)&sample_function);
    printf("Address of initialized_global  : %p\n", (void *)&initialized_global);
    printf("Address of uninitialized_global: %p\n", (void *)&uninitialized_global);
    printf("Address of static_global       : %p\n", (void *)&static_global);
    printf("Address of static_local        : %p\n", (void *)&static_local);
    printf("Address of local (stack)       : %p\n", (void *)&local);
    printf("Address of *heap_value         : %p\n", (void *)heap_value);

    free(heap_value);
    return 0;
}
