/*
 * greet.c - the second source file. It defines greet(), which main.c calls.
 *
 * It is compiled on its own into greet.o. The linker later joins greet.o
 * and main.o, and connects main's call to this function.
 */
#include <stdio.h>    /* declares printf; the code for printf is in the C library */
#include "greet.h"    /* our own header: checks that this definition matches it */

/* Print one greeting. printf is not in this file or in main.c: the linker
 * leaves it for the C library, which is connected when the program runs. */
void greet(int n)
{
    printf("hello number %d\n", n);
}
