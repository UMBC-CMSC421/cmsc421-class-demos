/*
 * Chapter 13: File Systems
 * Lab program: following a file through a File Allocation Table (FAT).
 *
 * A FAT is linked allocation with the links pulled out of the data blocks
 * and kept in one table. The table has one entry per disk block. An entry
 * holds the number of the file's next block, END for the file's last
 * block, or FREE for an unused block. The directory entry stores only the
 * file's first block.
 *
 * The program builds a 12-entry FAT holding two files, prints the table,
 * and then follows each file's chain from its first block.
 *
 * Build: make   (builds fat_trace)
 * Run:   ./fat_trace
 */

#include <stdio.h>      /* printf */

#define BLOCKS 12       /* the pretend disk has 12 blocks, so the FAT has 12 entries */
#define END -1          /* this block is the last block of its file */
#define FREE -2         /* this block belongs to no file */

/* Print a file's chain by starting at its first block and repeatedly
 * looking up the next block in the FAT until we reach END. */
void trace_file(int fat[], int start_block, const char *filename) {
    printf("%s starts at block %d\n", filename, start_block);
    printf("%s block chain: ", filename);

    int current = start_block;
    while (current != END) {
        printf("%d", current);
        current = fat[current];     /* the FAT entry names the next block */

        /* Print an arrow only between blocks, not after the last one. */
        if (current != END) {
            printf(" -> ");
        }
    }

    printf("\n\n");
}

int main(void) {
    int fat[BLOCKS];

    /* Step 1: start with every block free. */
    for (int i = 0; i < BLOCKS; i++) {
        fat[i] = FREE;
    }

    /* Step 2: store two files in the table. */
    // File A: 2 -> 7 -> 9
    fat[2] = 7;
    fat[7] = 9;
    fat[9] = END;

    // File B: 4 -> 5
    fat[4] = 5;
    fat[5] = END;

    /* Step 3: print the whole table, one row per block. */
    printf("Block | FAT entry\n");
    printf("------+----------\n");
    for (int i = 0; i < BLOCKS; i++) {
        if (fat[i] == FREE) {
            printf("%5d | FREE\n", i);
        } else if (fat[i] == END) {
            printf("%5d | END\n", i);
        } else {
            printf("%5d | %d\n", i, fat[i]);
        }
    }

    printf("\n");

    /* Step 4: follow each file from the first block its directory entry
     * would store. */
    trace_file(fat, 2, "File A");
    trace_file(fat, 4, "File B");

    return 0;
}
