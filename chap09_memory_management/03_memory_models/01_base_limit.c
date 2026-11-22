/*
 * Chapter 9: Main Memory
 * Demo 01: Check the limit before adding the base
 *
 * What's going on
 *   This textbook simulation uses a logical address measured from the start of
 *   a process. The limit is the size of its legal range. Check the limit first;
 *   only a legal address is translated by adding the base. This program models
 *   the hardware rule; it does not change the Linux process's address mappings.
 *
 * Build
 *   make
 * Run
 *   ./01_base_limit
 *   ./01_base_limit --step
 *   ./01_base_limit 1000
 * What to look for
 *   With limit 1000, address 999 is legal and address 1000 is not. Relocating
 *   the process changes the base and physical address, but not logical 346.
 */
#include <inttypes.h>
#include "walkthrough.h"

static void translate(uint32_t logical, uint32_t base, uint32_t limit, bool step)
{
    printf("\nLogical address = %" PRIu32 ", base = %" PRIu32 ", limit = %" PRIu32 "\n",
           logical, base, limit);
    pause_step(step);
    /* Do not add the base to an address outside the process's legal range. */
    if (logical >= limit) {
        printf("Check: %" PRIu32 " < %" PRIu32 "? no\nTRAP: outside the logical address range\n",
               logical, limit);
        return;
    }
    printf("Check: %" PRIu32 " < %" PRIu32 "? yes\n", logical, limit);
    pause_step(step);
    /* These fixed bases plus any accepted offset fit in uint32_t. */
    printf("Physical address = %" PRIu32 " + %" PRIu32 " = %" PRIu32 "\n",
           base, logical, base + logical);
}

int main(int argc, char **argv)
{
    Options opt = options(argc, argv, true);
    puts("Textbook simulation: base and limit");
    if (opt.has_address) {
        translate(opt.address, 14000, 1000, opt.step);
        return 0;  /* A simulated trap is an expected observation. */
    }
    translate(346, 14000, 1000, opt.step);
    translate(999, 14000, 1000, opt.step);
    translate(1000, 14000, 1000, opt.step);
    puts("\nRelocate the process. Keep the same logical address and limit.");
    translate(346, 22000, 1000, opt.step);
    return 0;
}
