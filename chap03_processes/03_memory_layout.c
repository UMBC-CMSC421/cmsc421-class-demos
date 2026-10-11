/*
 * Chapter 3: Processes
 * Demo 03: Where things live in a process's memory
 *
 * What's going on
 *   A process's memory is split into regions, and each one holds a certain kind of
 *   thing:
 *     text / read-only   the compiled code and constants. Nobody can write here.
 *     data               globals and statics that start with a value (= 7, = 42)
 *     bss                globals and statics with no starting value. They start at 0.
 *     heap               memory you get from malloc()
 *     stack              local variables of the functions that are running right now
 *   This program puts one variable in each region and prints its address.
 *
 * Build
 *   make
 *
 * Run
 *   ./03_memory_layout
 *   ./03_memory_layout --hold    same output, then it waits for Enter so you can look
 *                                at the process from another terminal
 *
 * What to look for
 *   The globals and the static local are all close together. That's the program's
 *   data area. The heap is above them, and the stack is way up at a much bigger
 *   address. Run it twice: the numbers change every time. That's ASLR (address space
 *   layout randomization). The kernel shuffles where regions go on each run so an
 *   attacker can't guess addresses.
 *
 * Other ways to see it
 *   ./03_memory_layout --hold, then in a second terminal with the PID it printed:
 *     cat /proc/<PID>/maps     one line per region, with an address range and its
 *                              permissions (r = read, w = write, x = execute). Find
 *                              [heap], [stack], and the lines for 03_memory_layout.
 *                              Every printed address falls inside one of them.
 *     pmap <PID>               the same list with sizes, a little easier to read
 *   setarch -R ./03_memory_layout
 *                              turns ASLR off for one run. Do it twice and the
 *                              addresses match.
 *   nm 03_memory_layout | grep -E 'global|static|read_only'
 *                              where the compiler put each variable: D or d = data,
 *                              B = bss, r = read-only. Lowercase means the name is
 *                              static (only visible in this file).
 */
#include <stdio.h>  /* printf(), perror(), getchar() */
#include <stdlib.h> /* malloc(), free() */
#include <string.h> /* strcmp() */
#include <unistd.h> /* getpid() */

/*
 * Anything declared outside a function lives in the program's data area, not on the
 * stack. It exists for the whole run. "static" out here means the name is private to
 * this file. "const" means the program can't change it.
 */
static int static_global = 42;                          /* has a starting value -> data */
int initialized_global = 7;                             /* has a starting value -> data */
int uninitialized_global;                               /* no starting value -> bss (0) */
static const char read_only_data[] = "read-only data";  /* const -> read-only area */

/*
 * argc is how many words were on the command line, counting the program name.
 * argv holds those words as strings. For "./03_memory_layout --hold", argc is 2
 * and argv[1] is "--hold".
 */
int main(int argc, char *argv[])
{
    int local = 99;                /* a normal local variable -> stack */
    static int static_local = 123; /* "static" inside a function -> data, NOT the stack.
                                      It keeps its value between calls. */

    /*
     * heap_value itself is a local, so the pointer is on the stack. The int it points
     * to came from malloc(), so that int is on the heap. We print where the int is.
     * sizeof(*heap_value) is the size in bytes of one int, so we ask for exactly that.
     */
    int *heap_value = malloc(sizeof(*heap_value));

    /* malloc() returns NULL if it can't get the memory. perror() prints why. */
    if (heap_value == NULL)
    {
        perror("malloc");
        return 1;
    }

    *heap_value = 555; /* the * means "the int this pointer points at" */

    /*
     * & means "the address of". %p prints an address, and it expects a void pointer,
     * which is why every one is cast to (void *).
     */
    printf("Address of read_only_data      : %p\n", (const void *)read_only_data);
    printf("Address of initialized_global  : %p\n", (void *)&initialized_global);
    printf("Address of uninitialized_global: %p\n", (void *)&uninitialized_global);
    printf("Address of static_global       : %p\n", (void *)&static_global);
    printf("Address of static_local        : %p\n", (void *)&static_local);
    printf("Address of local (stack)       : %p\n", (void *)&local);
    printf("Address of *heap_value         : %p\n", (void *)heap_value);

    /* strcmp() returns 0 when the two strings are the same. */
    if (argc == 2 && strcmp(argv[1], "--hold") == 0)
    {
        /*
         * getchar() waits for Enter. That keeps the process alive long enough to
         * inspect it. The (void) in front says we're ignoring what it returns on purpose.
         */
        printf("\nPID=%ld. Press Enter after inspecting this process with pmap or /proc/%ld/maps.\n",
               (long)getpid(), (long)getpid());
        (void)getchar();
    }
    else if (argc != 1)
    {
        /* Anything else is a typo. Error messages go to stderr, not regular output. */
        fprintf(stderr, "Usage: %s [--hold]\n", argv[0]);
        free(heap_value);
        return 2;
    }

    /* Hand the heap memory back. The kernel frees everything at exit anyway. */
    free(heap_value);
    return 0;
}
