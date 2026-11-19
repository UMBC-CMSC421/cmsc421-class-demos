/*
 * Chapter 13: File Systems
 * Lab program: three ways to allocate disk blocks to a file.
 *
 * A file is a list of logical blocks (0, 1, 2, ...). The file system
 * decides which physical disk block holds each one. This program shows
 * three methods on a pretend 16-block disk:
 *   contiguous  the file gets one run of neighboring blocks
 *   linked      each block stores the number of the next block
 *   indexed     one index block lists every data block of the file
 *
 * Build: make   (builds allocation_sim)
 * Run:   ./allocation_sim
 */

#include <stdio.h>      /* printf */

#define BLOCKS 16       /* size of the pretend disk, in blocks */

/* Print a two-line map of the disk: block numbers on top, and under each
 * one an X if the block is used or a . if it is free. */
void print_disk(int disk[]) {
    printf("Block: ");
    for (int i = 0; i < BLOCKS; i++) printf("%2d ", i);
    printf("\nUsed:  ");
    for (int i = 0; i < BLOCKS; i++) printf("%2c ", disk[i] ? 'X' : '.');
    printf("\n\n");
}

/* Contiguous allocation: the directory stores only a start block and a
 * length. Logical block N is physical block start + N. */
void contiguous_demo(void) {
    int disk[BLOCKS] = {0};     /* 0 = free, 1 = used; every block starts free */

    printf("=== Contiguous Allocation ===\n");
    printf("File A needs 4 consecutive blocks.\n");

    int start = 3;      /* first physical block of File A */
    int length = 4;     /* File A uses 4 blocks: 3, 4, 5, 6 */

    /* Mark File A's blocks as used. */
    for (int i = start; i < start + length; i++) {
        disk[i] = 1;
    }

    print_disk(disk);

    /* Finding any block is one addition, so direct access is fast. */
    printf("File A block mapping:\n");
    for (int logical = 0; logical < length; logical++) {
        printf("logical block %d -> physical block %d\n", logical, start + logical);
    }

    printf("Observation: Fast direct access, but requires a consecutive free region.\n\n");
}

/* Linked allocation: the directory stores the first block, and each block
 * stores the number of the next one. next[] plays the role of those
 * pointers. -1 marks the last block of the file; -2 marks a block that is
 * not part of this file. */
void linked_demo(void) {
    int next[BLOCKS];
    for (int i = 0; i < BLOCKS; i++) next[i] = -2;     /* no block is in use yet */

    printf("=== Linked Allocation ===\n");
    printf("File B uses scattered blocks: 2 -> 9 -> 5 -> 13\n");

    /* Build File B's chain. The blocks do not need to be neighbors. */
    next[2] = 9;
    next[9] = 5;
    next[5] = 13;
    next[13] = -1;      /* end of file */

    int current = 2;    /* the directory entry says File B starts at block 2 */
    int logical = 0;

    /* Walk the chain one block at a time. To reach logical block N we must
     * visit the N blocks before it, so random access is slow. */
    while (current != -1) {
        printf("logical block %d -> physical block %d\n", logical, current);
        current = next[current];    /* follow the pointer to the next block */
        logical++;
    }

    printf("Observation: No need for contiguous space, but random access is slow.\n\n");
}

/* Indexed allocation: the directory stores the number of one index block.
 * The index block holds an array: entry N is the physical block of logical
 * block N. file_blocks[] plays the role of that array. */
void indexed_demo(void) {
    printf("=== Indexed Allocation ===\n");
    printf("File C has an index block containing pointers to data blocks.\n");

    int index_block = 4;                    /* where the index itself lives */
    int file_blocks[] = {1, 6, 10, 15};     /* contents of the index block */
    int length = 4;

    printf("Index block: %d\n", index_block);

    /* Any logical block is one array lookup away: fast direct access. The
     * cost is one extra block (the index) for every file. */
    for (int logical = 0; logical < length; logical++) {
        printf("index[%d] = physical block %d\n", logical, file_blocks[logical]);
    }

    printf("Observation: Good direct access, but index block adds overhead.\n\n");
}

int main(void) {
    contiguous_demo();
    linked_demo();
    indexed_demo();
    return 0;
}
