/*
 * greet.h - the header shared by main.c and greet.c.
 *
 * A header holds declarations: it tells the compiler a function exists
 * and what its arguments are, so main.c can call greet() without seeing
 * its code. The preprocessor pastes this whole file into every .c file
 * that says #include "greet.h".
 */
#ifndef GREET_H      /* include guard: skip the file if it was already pasted in */
#define GREET_H

#define TIMES 3      /* a macro: the preprocessor replaces every TIMES with 3 */

void greet(int n);   /* declaration only; the definition is in greet.c */

#endif
