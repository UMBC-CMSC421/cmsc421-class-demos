/*
 * Chapter 9: Main Memory
 * Demo 06: Walk a two-level page table
 *
 * What's going on
 *   This simulation divides a 32-bit address into 10 outer-index bits,
 *   10 inner-index bits, and 12 offset bits. Only populated regions receive
 *   a second-level table. The model uses 4-byte entries and 4 KiB pages.
 *   It is the textbook's two-level model, not the course VM's x86-64 walker.
 *
 * Build
 *   make
 * Run
 *   ./06_two_level
 *   ./06_two_level --step
 *   ./06_two_level 0x00403ABC
 * What to look for
 *   0x00403ABC splits into outer 1, inner 3, offset 0xABC. Frame 0x1F7
 *   gives physical 0x1F7ABC. Missing outer and inner entries stop the walk.
 *   Two populated regions use 12 KiB of modeled tables instead of 4 MiB.
 */
#include <inttypes.h>
#include "walkthrough.h"

#define ENTRIES 1024U
#define PAGE_SIZE 4096U
/* C pointers connect the model's tables. The printed sizes use the textbook's
 * 4-byte directory entries, not sizeof(pointer) on the host running this code. */
static uint32_t *directory[ENTRIES];
static unsigned leaf_tables;

static void map_page(uint32_t virtual_address, uint32_t frame)
{
    uint32_t outer = virtual_address >> 22;
    uint32_t inner = (virtual_address >> 12) & 0x3FFU;
    if (directory[outer] == NULL) {
        directory[outer] = calloc(ENTRIES, sizeof(uint32_t));
        if (directory[outer] == NULL) { perror("calloc"); exit(EXIT_FAILURE); }
        leaf_tables++;
    }
    /* Bit 0 marks a present entry. The upper 20 bits hold the frame number. */
    directory[outer][inner] = (frame << 12) | 1U;
}

static void walk(uint32_t address, bool step)
{
    uint32_t outer = address >> 22;
    uint32_t inner = (address >> 12) & 0x3FFU;
    uint32_t offset = address & 0xFFFU;
    printf("\nLogical 0x%08" PRIX32 " -> outer %" PRIu32 ", inner %" PRIu32 ", offset 0x%03" PRIX32 "\n",
           address, outer, inner, offset);
    pause_step(step);
    if (directory[outer] == NULL) {
        puts("STOP: outer entry absent; there is no second-level table"); return;
    }
    printf("Directory[%" PRIu32 "] selects a second-level table\n", outer);
    pause_step(step);
    uint32_t entry = directory[outer][inner];
    if (!(entry & 1U)) {
        puts("STOP: inner entry absent; there is no mapped data page"); return;
    }
    uint32_t frame = entry >> 12;
    printf("Table[%" PRIu32 "] selects frame 0x%" PRIX32 "\n", inner, frame);
    pause_step(step);
    printf("Physical = 0x%" PRIX32 " * 4096 + 0x%03" PRIX32 " = 0x%" PRIX32 "\n",
           frame, offset, frame * PAGE_SIZE + offset);
}

int main(int argc, char **argv)
{
    Options opt = options(argc, argv, true);
    map_page(0x00403000, 0x1F7);
    map_page(0x00801000, 0x220);
    puts("Textbook simulation: 32-bit addresses, split 10 | 10 | 12");
    walk(opt.has_address ? opt.address : 0x00403ABC, opt.step);
    if (!opt.has_address) {
        walk(0x00801005, opt.step);
        walk(0x00404000, opt.step); /* Existing table, missing page. */
        walk(0x00C01000, opt.step); /* Missing table. */
    }
    printf("\nModeled table bytes: directory 4096 + %u leaf tables * 4096 = %u\n",
           leaf_tables, (1U + leaf_tables) * PAGE_SIZE);
    puts("Flat table bytes: 1048576 entries * 4 = 4194304");
    puts("These totals exclude data frames and describe this sparse mapping.");
    for (size_t i = 0; i < ENTRIES; i++) free(directory[i]);
    return 0;
}
