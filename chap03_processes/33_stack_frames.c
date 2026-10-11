/*
 * Chapter 3: Processes
 * Demo 33: Where stack frames live (three nested calls)
 *
 * What's going on
 *   Every function call gets its own stack frame: a slice of the stack that holds
 *   that call's local variables and its return address. main calls functionA,
 *   functionA calls functionB, and functionB calls functionC, so at the deepest
 *   point four frames sit on the stack at once.
 *   Each function prints the address of one of its local variables. That address
 *   is inside the function's own frame, so it shows where the frame is in memory.
 *   For contrast, main also prints the address of a global, which lives in the
 *   data section, far away from the stack.
 *
 * Build
 *   make                      builds every demo in this folder
 *
 * Run
 *   ./33_stack_frames
 *
 * What to look for
 *   The stack addresses get smaller with each call: functionA's frame is below
 *   main's, functionB's is below functionA's, and so on. That is the stack growing
 *   down. The gaps between them are the frame sizes.
 *   The global's address is nowhere near the others, because it is not on the stack.
 *   Run it twice: the numbers change (address space layout randomization), but the
 *   order, and the gaps, stay the same.
 *
 * Other ways to see it
 *   setarch -R ./33_stack_frames      turn off randomization: same numbers every run
 *   gdb ./33_stack_frames, then: break functionC, run, backtrace
 *                                     gdb lists the four frames, newest first
 */
#include <stdio.h> /* printf() */

int global_counter = 0; /* a global: lives in the data section, not on the stack */

void functionC(int x, int y)
{
    int sum = x + y; /* local: in functionC's frame */
    printf("functionC frame: &sum    = %p\n", (void *)&sum);
    printf("Sum: %d\n", sum);
} /* functionC returns: its frame is popped */

void functionB(int a)
{
    int square = a * a; /* local: in functionB's frame */
    printf("functionB frame: &square = %p\n", (void *)&square);
    functionC(a, square); /* pushes a new frame for functionC */
} /* functionB returns: its frame is popped */

void functionA(void)
{
    int num = 10; /* local: in functionA's frame */
    printf("functionA frame: &num    = %p\n", (void *)&num);
    functionB(num); /* pushes a new frame for functionB */
} /* functionA returns: its frame is popped */

int main(void)
{
    int marker = 0; /* local: in main's frame */
    /*
     * %p prints an address in hex. The (void *) cast is what printf expects for %p.
     */
    printf("global (data):   &global_counter = %p\n", (void *)&global_counter);
    printf("main frame:      &marker = %p\n", (void *)&marker);
    functionA(); /* pushes a new frame for functionA */
    return marker;
}
