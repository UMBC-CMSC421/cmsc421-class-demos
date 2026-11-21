/*
 * Chapter 9: Main Memory
 * Demo 04: A TLB caches translations
 *
 * What's going on
 *   This simulation adds a two-entry, FIFO TLB in front of a one-level page
 *   table. Every valid page is already resident. Entries carry a process tag
 *   so process B cannot reuse process A's translation for the same page number.
 *   FIFO and the one-level access counts are model choices, not measurements
 *   of the course CPU's TLB or its multilevel page-table walker.
 *
 * Build
 *   make
 * Run
 *   ./04_tlb
 *   ./04_tlb --step
 * What to look for
 *   Addresses 13 and 12 share a page, so the second access hits. The first
 *   six accesses produce three hits and three misses. A miss reads the page
 *   table; it is not a page fault here because the valid pages are resident.
 */
#include <inttypes.h>
#include "walkthrough.h"

typedef struct { bool valid; unsigned process; uint32_t page, frame; } Entry;
static Entry tlb[2];
static size_t next_victim;
static unsigned hits, misses;
static const uint32_t tables[2][4] = {{5,6,1,2},{5,6,1,7}};

static void access_byte(unsigned process, uint32_t address, bool step)
{
    printf("\nProcess %c, logical address %" PRIu32 "\n", (char)('A' + process), address);
    if (address >= 16) {
        puts("INVALID: outside the logical address space; no data access counted");
        return;
    }
    uint32_t page = address / 4, offset = address % 4, frame = 0;
    printf("Split: page %" PRIu32 ", offset %" PRIu32 "\n", page, offset);
    pause_step(step);
    bool found = false;
    /* Both the process tag and page number must match. */
    for (size_t i = 0; i < 2; i++) {
        if (tlb[i].valid && tlb[i].process == process && tlb[i].page == page) {
            frame = tlb[i].frame;
            found = true;
            break;
        }
    }
    if (found) {
        hits++;
        puts("TLB HIT: use the cached frame; skip the page table");
    } else {
        misses++;
        frame = tables[process][page];
        printf("TLB MISS: page table says frame %" PRIu32 "; page is resident\n", frame);
        /* FIFO replaces the oldest inserted slot. Hits do not change its order. */
        tlb[next_victim] = (Entry){true, process, page, frame};
        next_victim = (next_victim + 1) % 2;
    }
    printf("Physical address = %" PRIu32 "\n", frame * 4 + offset);
    for (size_t i = 0; i < 2; i++) {
        if (tlb[i].valid) printf("  slot %zu: process %c, page %" PRIu32 " -> frame %" PRIu32 "\n",
                                i, (char)('A' + tlb[i].process), tlb[i].page, tlb[i].frame);
        else printf("  slot %zu: empty\n", i);
    }
}

int main(int argc, char **argv)
{
    Options opt = options(argc, argv, false);
    const uint32_t refs[] = {13,12,6,13,0,6};
    puts("Textbook simulation: 2-entry FIFO TLB with process tags");
    for (size_t i = 0; i < sizeof refs / sizeof refs[0]; i++) access_byte(0, refs[i], opt.step);
    unsigned accesses = hits + misses;
    printf("\nProcess A trace: hits = %u; misses = %u; data accesses = %u; table reads = %u\n",
           hits, misses, accesses, misses);
    /* Count one data read per request, plus one table read for each miss. */
    printf("Memory reads = %u; at 10 ns per read, modeled EAT = %.1f ns\n",
           accesses + misses, 10.0 * (double)(accesses + misses) / (double)accesses);
    puts("This model ignores TLB lookup time and data caches.");
    pause_step(opt.step);
    puts("\nSwitch processes without flushing: tags keep the address spaces separate");
    access_byte(1, 13, opt.step);
    access_byte(0, 13, opt.step);
    access_byte(0, 16, opt.step);
    return 0;
}
