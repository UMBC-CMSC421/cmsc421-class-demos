/*
 * main.c - the first source file. It calls greet(), which lives in greet.c.
 *
 * run_steps.sh builds this program one step at a time:
 * preprocess, compile, assemble, link, load, run.
 */
#include "greet.h"    /* gives us TIMES and the declaration of greet() */

int counter = 0;      /* a global variable: it gets an entry in the symbol table */

int main(void)
{
    /* The preprocessor turns TIMES into 3 before the compiler sees this line. */
    for (int i = 1; i <= TIMES; i++) {
        greet(i);     /* defined in greet.c; the linker fills in its address */
        counter++;
    }
    return 0;
}
