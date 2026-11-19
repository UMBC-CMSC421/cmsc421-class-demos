/*
 * Chapter 9: Main Memory
 * Demo 02, prog1.c: one global and one library call, to follow through the build.
 *
 * Day 1 commands (run in this folder on the course VM):
 *   gcc -Wall -c prog1.c         compile and assemble only: makes prog1.o
 *   nm prog1.o                   symbol table: B counter, T main, U puts
 *   gcc -Wall -o prog1 prog1.o   link: makes the executable prog1
 *   nm prog1                     main and counter now have offsets; puts is still U
 *   ldd ./prog1                  the shared libraries the loader will map
 *   ./prog1                      load and run
 *
 * gcc turns printf("Hello\n") into a call to puts, because the string has no
 * format codes and ends in a newline. That is why nm shows puts, not printf.
 */
#include <stdio.h>

int counter = 0;    /* global, starts at zero: nm shows it in .bss (letter B) */

int main(void)      /* global function: nm shows it in .text (letter T) */
{
    printf("Hello\n");  /* defined in libc, not here: nm shows puts as U */
    return 0;
}
