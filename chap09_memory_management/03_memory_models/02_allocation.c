/*
 * Chapter 9: Main Memory
 * Demo 02: Allocate blocks and expose external fragmentation
 *
 * What's going on
 *   This simulation represents memory as an ordered list of blocks. A zero
 *   owner marks a free hole. Allocation can split a hole; freeing a block
 *   merges adjacent holes. First, best, and worst fit start from the same map.
 *   This is the textbook's contiguous-allocation model, not Linux malloc().
 *
 * Build
 *   make
 * Run
 *   ./02_allocation
 *   ./02_allocation --step
 * What to look for
 *   The 212-unit request selects holes of 500, 300, and 600 units. Later,
 *   holes of 3 and 5 units cannot hold a contiguous request of 6, even though
 *   8 units are free. Compaction joins the holes and lets the request succeed.
 */
#include <assert.h>
#include "walkthrough.h"

#define CAPACITY 32
typedef struct { int size; char owner; } Block;
typedef struct { Block block[CAPACITY]; size_t count; } Memory;
typedef enum { FIRST, BEST, WORST } Policy;

static int total(const Memory *m)
{
    int size = 0;
    for (size_t i = 0; i < m->count; i++) size += m->block[i].size;
    return size;
}

static void show(const Memory *m)
{
    int start = 0, free_units = 0, largest = 0;
    for (size_t i = 0; i < m->count; i++) {
        Block b = m->block[i];
        printf("  [%d,%d) %s", start, start + b.size, b.owner ? "process " : "hole");
        if (b.owner) printf("%c", b.owner);
        printf(" (%d units)\n", b.size);
        start += b.size;
        if (!b.owner) {
            free_units += b.size;
            if (b.size > largest) largest = b.size;
        }
    }
    printf("Total free = %d; largest hole = %d\n", free_units, largest);
}

static bool allocate(Memory *m, int request, char owner, Policy policy)
{
    size_t chosen = m->count; /* count is an out-of-range sentinel: no fit yet. */
    for (size_t i = 0; i < m->count; i++) {
        Block b = m->block[i];
        if (b.owner || b.size < request) continue;
        if (chosen == m->count ||
            (policy == BEST && b.size < m->block[chosen].size) ||
            (policy == WORST && b.size > m->block[chosen].size)) chosen = i;
        if (policy == FIRST) break; /* First fit stops as soon as it can fit. */
    }
    if (chosen == m->count) {
        printf("Request %d: NO FIT\n", request);
        return false;
    }
    int original_total = total(m);
    int old_size = m->block[chosen].size;
    printf("Request %d: selected hole %d; remainder %d\n", request, old_size, old_size - request);
    if (old_size > request) {
        /* Make room for the unused tail immediately after the new allocation. */
        assert(m->count < CAPACITY);
        memmove(&m->block[chosen + 2], &m->block[chosen + 1],
                (m->count - chosen - 1) * sizeof(Block));
        m->block[chosen + 1] = (Block){old_size - request, 0};
        m->count++;
    }
    m->block[chosen] = (Block){request, owner};
    assert(total(m) == original_total); /* Allocation never creates memory. */
    return true;
}

static void release(Memory *m, char owner)
{
    for (size_t i = 0; i < m->count; i++)
        if (m->block[i].owner == owner) m->block[i].owner = 0;
    /* Recheck the same position after a merge: there may be a third hole. */
    for (size_t i = 0; i + 1 < m->count;) {
        if (!m->block[i].owner && !m->block[i + 1].owner) {
            m->block[i].size += m->block[i + 1].size;
            memmove(&m->block[i + 1], &m->block[i + 2],
                    (m->count - i - 2) * sizeof(Block));
            m->count--;
        } else i++;
    }
}

static void compact(Memory *m)
{
    size_t used = 0;
    int free_units = 0;
    /* Keep allocated blocks in order; collect all free space at the end.
     * A real implementation must move bytes and update relocation information. */
    for (size_t i = 0; i < m->count; i++) {
        if (m->block[i].owner) m->block[used++] = m->block[i];
        else free_units += m->block[i].size;
    }
    if (free_units) m->block[used++] = (Block){free_units, 0};
    m->count = used;
}

int main(int argc, char **argv)
{
    Options opt = options(argc, argv, false);
    const Memory initial = {{{100,0},{1,'A'},{500,0},{1,'B'},{200,0},
                              {1,'C'},{300,0},{1,'D'},{600,0}},9};
    const char *names[] = {"FIRST FIT", "BEST FIT", "WORST FIT"};
    puts("Textbook simulation: contiguous allocation (abstract units)");
    puts("The one-unit allocated blocks separate the five holes.");
    show(&initial);
    for (int p = FIRST; p <= WORST; p++) {
        Memory m = initial; /* Reset so each policy sees the same starting map. */
        printf("\n%s\n", names[p]);
        pause_step(opt.step);
        if (!allocate(&m, 212, 'N', (Policy)p)) return 1;
        show(&m);
    }
    Memory m = {{{3,'A'},{4,'B'},{5,'C'},{4,'D'}},4};
    puts("\nExternal fragmentation: start with four processes"); show(&m);
    pause_step(opt.step);
    puts("Free A and C"); release(&m, 'A'); release(&m, 'C'); show(&m);
    if (allocate(&m, 6, 'N', FIRST)) return 1; /* Neither hole can hold six. */
    pause_step(opt.step);
    puts("Compact: move B and D together"); compact(&m); show(&m);
    if (!allocate(&m, 6, 'N', FIRST)) return 1;
    show(&m);
    pause_step(opt.step);
    puts("Free N: its six units merge with the adjacent two-unit hole");
    release(&m, 'N'); show(&m);
    assert(total(&m) == 16);
    return 0;
}
