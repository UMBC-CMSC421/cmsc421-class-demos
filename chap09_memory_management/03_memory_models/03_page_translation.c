/*
 * Chapter 9: Main Memory
 * Demo 03: Follow a byte through a page table
 *
 * What's going on
 *   This simulation stores the letters a through p in nonadjacent frames.
 *   Divide a logical address into page and offset, look up the frame, and
 *   keep the offset. The small model has 4-byte pages and a 16-byte address
 *   space. A separate example uses 4 KiB pages and a hexadecimal address.
 *
 * Build
 *   make
 * Run
 *   ./03_page_translation
 *   ./03_page_translation --step
 *   ./03_page_translation 13
 * What to look for
 *   Logical 13 is page 3, offset 1. Page 3 is in frame 2, so physical 9 holds
 *   n. Another process can map logical 13 elsewhere. This program's physical
 *   addresses are indexes in a simulated memory array, not Linux PFNs.
 */
#include <inttypes.h>
#include "walkthrough.h"

static void translate(uint32_t address, const uint32_t table[4], const char ram[32], bool step)
{
    const uint32_t page_size = 4;
    printf("\nLogical address = %" PRIu32 "; page size = 4 bytes\n", address);
    if (address >= 16) {
        puts("INVALID: outside this process's 16-byte logical address space");
        return;
    }
    pause_step(step);
    uint32_t page = address / page_size;  /* Which page contains this byte? */
    uint32_t offset = address % page_size; /* How far into that page? */
    printf("Split: page = %" PRIu32 " / 4 = %" PRIu32 "; offset = %" PRIu32 " %% 4 = %" PRIu32 "\n",
           address, page, address, offset);
    pause_step(step);
    uint32_t frame = table[page];
    printf("Lookup: page_table[%" PRIu32 "] = frame %" PRIu32 "\n", page, frame);
    pause_step(step);
    uint32_t physical = frame * page_size + offset;
    printf("Join: %" PRIu32 " * 4 + %" PRIu32 " = %" PRIu32 "\n", frame, offset, physical);
    printf("Read: physical_memory[%" PRIu32 "] = '%c'\n", physical, ram[physical]);
}

int main(int argc, char **argv)
{
    Options opt = options(argc, argv, true);
    const uint32_t table_a[4] = {5, 6, 1, 2};
    const uint32_t table_b[4] = {5, 6, 1, 7};
    const char letters[] = "abcdefghijklmnop";
    char ram[32];
    memset(ram, '.', sizeof ram);
    /* Place each logical byte according to process A's table. */
    for (uint32_t a = 0; a < 16; a++) ram[table_a[a / 4] * 4 + a % 4] = letters[a];
    ram[7 * 4 + 1] = 'X';
    puts("Textbook simulation: page-table translation");
    puts("Process A: page 0 -> frame 5, page 1 -> frame 6, page 2 -> frame 1, page 3 -> frame 2");
    if (opt.has_address) {
        translate(opt.address, table_a, ram, opt.step);
        return 0;
    }
    const uint32_t addresses[] = {13, 3, 4, 6, 15, 16};
    for (size_t i = 0; i < sizeof addresses / sizeof addresses[0]; i++)
        translate(addresses[i], table_a, ram, opt.step);
    puts("\nSwitch to process B: its page 3 maps to frame 7");
    translate(13, table_b, ram, opt.step);
    pause_step(opt.step);
    uint32_t address = 0x2A7C, page_size = 4096, frame = 3;
    uint32_t page = address / page_size, offset = address % page_size;
    puts("\nSeparate 16-bit model: 4 KiB pages, page 2 maps to frame 3");
    printf("0x%04" PRIX32 " -> page 0x%" PRIX32 ", offset 0x%03" PRIX32 "\n", address, page, offset);
    printf("Physical = 3 * 4096 + %" PRIu32 " = 0x%04" PRIX32 "\n", offset, frame * page_size + offset);
    pause_step(opt.step);
    puts("\nInternal fragmentation: a separate 2049-byte allocation with 1024-byte frames");
    uint32_t bytes = 2049, size = 1024;
    uint32_t pages = (bytes + size - 1) / size; /* Round up to whole frames. */
    printf("Frames = %" PRIu32 "; allocated = %" PRIu32 "; unused tail = %" PRIu32 " bytes\n",
           pages, pages * size, pages * size - bytes);
    return 0;
}
