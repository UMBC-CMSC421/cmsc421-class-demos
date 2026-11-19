/*
 * Chapter 13: File Systems
 * Lab program: free-space management with a bitmap.
 *
 * The file system keeps one bit per disk block to record which blocks are
 * free. In this program 1 means free and 0 means allocated.
 *
 * The program starts with a 16-block disk that is partly used, then
 * allocates a 4-block file by taking the first 4 free blocks it finds
 * (first fit), and prints the bitmap before and after.
 *
 * Build: make   (builds bitmap_alloc)
 * Run:   ./bitmap_alloc
 */

#include <stdio.h>      /* printf */

#define BLOCKS 16       /* size of the pretend disk, in blocks */

/* Print block numbers on one line and each block's bit under it. */
void print_bitmap(int bitmap[]) {
    printf("Block:  ");
    for (int i = 0; i < BLOCKS; i++) printf("%2d ", i);
    printf("\nBitmap: ");
    for (int i = 0; i < BLOCKS; i++) printf("%2d ", bitmap[i]);
    printf("\n\n");
}

/* Scan the bitmap from block 0 and take free blocks until we have
 * `needed` of them. Each block taken is marked allocated (set to 0) and
 * its number is saved in allocated[].
 * Returns 1 if we found enough free blocks, 0 if not. */
int allocate_first_fit(int bitmap[], int needed, int allocated[]) {
    int count = 0;      /* how many blocks we have taken so far */

    for (int i = 0; i < BLOCKS && count < needed; i++) {
        if (bitmap[i] == 1) {           /* block i is free */
            bitmap[i] = 0;              /* mark it allocated */
            allocated[count] = i;       /* remember which block we took */
            count++;
        }
    }

    return count == needed;
}

int main(void) {
    /* Step 1: the starting bitmap, 4 blocks per row (1 = free, 0 = allocated).
     * Free blocks: 0, 3, 4, 6, 9, 10, 11, 13, 15. */
    int bitmap[BLOCKS] = {
        1, 0, 0, 1,
        1, 0, 1, 0,
        0, 1, 1, 1,
        0, 1, 0, 1
    };

    int allocated[4];   /* receives the block numbers given to the new file */

    printf("Initial free-space bitmap:\n");
    print_bitmap(bitmap);

    /* Step 2: allocate 4 blocks. First fit takes 0, 3, 4, 6. The blocks
     * do not need to be neighbors. */
    printf("Allocating a 4-block file using first-fit free block selection...\n\n");

    if (allocate_first_fit(bitmap, 4, allocated)) {
        printf("Allocated blocks: ");
        for (int i = 0; i < 4; i++) {
            printf("%d ", allocated[i]);
        }
        printf("\n\n");
    } else {
        printf("Not enough free blocks.\n");
    }

    /* Step 3: the blocks we took now show 0. */
    printf("Updated bitmap:\n");
    print_bitmap(bitmap);

    return 0;
}
