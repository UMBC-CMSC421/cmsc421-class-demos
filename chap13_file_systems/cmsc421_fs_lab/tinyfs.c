/*
 * Chapter 13: File Systems
 * Lab program: TinyFS, a small file system that lives in memory.
 *
 * TinyFS has the three parts every file system needs:
 *   a directory      maps a file name to the file's metadata (FileEntry)
 *   a free-space bitmap  records which disk blocks are free
 *   the disk blocks  hold the file data
 * Here the "disk" is an array of 64 blocks of 64 bytes each, so nothing
 * is saved after the program exits. A file gets a fixed number of blocks
 * when it is created, and the directory entry lists them (indexed
 * allocation).
 *
 * The program reads commands at an fs> prompt. Type help for the list.
 *
 * Build: make   (builds tinyfs)
 * Run:   ./tinyfs
 *        fs> create notes 2
 *        fs> write notes hello file systems
 *        fs> stat notes
 *        fs> bitmap
 *        fs> read notes
 *        fs> quit
 */

#include <stdio.h>      /* printf, fgets, fwrite */
#include <string.h>     /* strcmp, strncpy, strlen, strtok, memset, memcpy */
#include <stdlib.h>     /* atoi */

#define MAX_FILES 16        /* the directory has room for 16 files */
#define MAX_BLOCKS 64       /* the disk has 64 blocks */
#define MAX_NAME 32         /* longest file name, including the '\0' */
#define MAX_FILE_BLOCKS 8   /* one file may use at most 8 blocks */
#define BLOCK_SIZE 64       /* each block holds 64 bytes */
#define INPUT_SIZE 256      /* longest command line we read */

/* One directory entry: everything TinyFS knows about one file.
 * A real file system keeps this kind of metadata in a file control block
 * (an inode on Linux). */
typedef struct {
    int used;                       /* 1 if this entry holds a file, 0 if empty */
    char name[MAX_NAME];            /* the file's name */
    int size_blocks;                /* how many blocks the file owns */
    int blocks[MAX_FILE_BLOCKS];    /* which disk blocks, in order (-1 = none) */
    int size_bytes;                 /* how many bytes of data are stored */
} FileEntry;

/* The whole file system. `static` keeps these private to this file. */
static FileEntry directory[MAX_FILES];
static int bitmap[MAX_BLOCKS];              // 1 = free, 0 = allocated
static char disk[MAX_BLOCKS][BLOCK_SIZE];   // simulated disk blocks

/* "Format" the disk: mark every block free, zero its bytes, and empty
 * every directory entry. */
void init_fs(void) {
    for (int i = 0; i < MAX_BLOCKS; i++) {
        bitmap[i] = 1;
        memset(disk[i], 0, BLOCK_SIZE);
    }

    for (int i = 0; i < MAX_FILES; i++) {
        directory[i].used = 0;
        directory[i].name[0] = '\0';
        directory[i].size_blocks = 0;
        directory[i].size_bytes = 0;

        for (int j = 0; j < MAX_FILE_BLOCKS; j++) {
            directory[i].blocks[j] = -1;
        }
    }
}

/* Look up a name in the directory (a linear search, like a simple
 * list-based directory). Returns the entry's index, or -1 if not found. */
int find_file(const char *name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (directory[i].used && strcmp(directory[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/* Find an empty directory slot for a new file. Returns -1 if all 16 are used. */
int find_free_entry(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!directory[i].used) {
            return i;
        }
    }
    return -1;
}

/* Count the 1 bits in the bitmap: the number of free blocks. */
int count_free_blocks(void) {
    int count = 0;
    for (int i = 0; i < MAX_BLOCKS; i++) {
        if (bitmap[i] == 1) {
            count++;
        }
    }
    return count;
}

/* Take `needed` free blocks, first fit, and write their numbers into
 * out_blocks[]. Checks the free count first so we never take only some of
 * the blocks and then fail. Returns 1 on success, 0 if the disk is too full. */
int allocate_blocks(int needed, int out_blocks[]) {
    if (needed > count_free_blocks()) {
        return 0;
    }

    int count = 0;

    for (int i = 0; i < MAX_BLOCKS && count < needed; i++) {
        if (bitmap[i] == 1) {           /* block i is free */
            bitmap[i] = 0;              /* mark it allocated */
            out_blocks[count] = i;
            count++;
        }
    }

    return count == needed;
}

/* Give a file's blocks back: mark each one free in the bitmap and zero its
 * data. (A real file system usually only updates the bitmap and leaves the
 * old bytes on disk.) */
void free_file_blocks(FileEntry *file) {
    for (int i = 0; i < file->size_blocks; i++) {
        int block = file->blocks[i];

        if (block >= 0 && block < MAX_BLOCKS) {     /* skip unused slots */
            bitmap[block] = 1;
            memset(disk[block], 0, BLOCK_SIZE);
            file->blocks[i] = -1;
        }
    }
}

/* create <name> <blocks>: make a new, empty file that owns `blocks` blocks. */
void cmd_create(char *name, int blocks) {
    /* Check 1: the block count must be 1 to 8. */
    if (blocks <= 0 || blocks > MAX_FILE_BLOCKS) {
        printf("Error: blocks must be between 1 and %d.\n", MAX_FILE_BLOCKS);
        return;
    }

    /* Check 2: names in one directory must be unique. */
    if (find_file(name) >= 0) {
        printf("Error: file '%s' already exists.\n", name);
        return;
    }

    /* Check 3: the directory needs an empty slot. */
    int entry = find_free_entry();
    if (entry < 0) {
        printf("Error: directory is full.\n");
        return;
    }

    int allocated[MAX_FILE_BLOCKS];

    /* Check 4: the disk needs enough free blocks. */
    if (!allocate_blocks(blocks, allocated)) {
        printf("Error: not enough free blocks.\n");
        return;
    }

    /* Fill in the directory entry. strncpy copies at most MAX_NAME - 1
     * characters; the next line makes sure the name ends with '\0'. */
    directory[entry].used = 1;
    strncpy(directory[entry].name, name, MAX_NAME - 1);
    directory[entry].name[MAX_NAME - 1] = '\0';
    directory[entry].size_blocks = blocks;
    directory[entry].size_bytes = 0;    /* the file is empty until write */

    /* Record which blocks the file owns: this list is the file's index. */
    for (int i = 0; i < blocks; i++) {
        directory[entry].blocks[i] = allocated[i];
    }

    printf("Created file '%s' using %d block(s).\n", name, blocks);
}

/* delete <name>: free the file's blocks, then clear its directory entry. */
void cmd_delete(char *name) {
    int idx = find_file(name);

    if (idx < 0) {
        printf("Error: file '%s' not found.\n", name);
        return;
    }

    free_file_blocks(&directory[idx]);

    directory[idx].used = 0;
    directory[idx].name[0] = '\0';
    directory[idx].size_blocks = 0;
    directory[idx].size_bytes = 0;

    printf("Deleted file '%s'.\n", name);
}

/* ls: print one line per file in the directory. */
void cmd_ls(void) {
    printf("Name                           Blocks   Bytes\n");
    printf("------------------------------ -------- ------\n");

    int any = 0;    /* set to 1 once we print a file */

    for (int i = 0; i < MAX_FILES; i++) {
        if (directory[i].used) {
            printf("%-30s %8d %6d\n",
                   directory[i].name,
                   directory[i].size_blocks,
                   directory[i].size_bytes);
            any = 1;
        }
    }

    if (!any) {
        printf("(empty directory)\n");
    }
}

/* bitmap: print the free-space bitmap, 16 blocks per row. */
void cmd_bitmap(void) {
    printf("Block:  ");
    for (int i = 0; i < MAX_BLOCKS; i++) {
        printf("%2d ", i);
        if ((i + 1) % 16 == 0) printf("\n        ");     /* start a new row */
    }

    printf("\nBitmap: ");
    for (int i = 0; i < MAX_BLOCKS; i++) {
        printf("%2d ", bitmap[i]);
        if ((i + 1) % 16 == 0) printf("\n        ");
    }

    printf("\nLegend: 1 = free, 0 = allocated\n");
}

/* stat <name>: print the file's metadata from its directory entry. */
void cmd_stat(char *name) {
    int idx = find_file(name);

    if (idx < 0) {
        printf("Error: file '%s' not found.\n", name);
        return;
    }

    FileEntry *file = &directory[idx];  /* a pointer, so we do not copy the entry */

    printf("File: %s\n", file->name);
    printf("Size in blocks: %d\n", file->size_blocks);
    printf("Size in bytes:  %d\n", file->size_bytes);
    printf("Blocks: ");

    for (int i = 0; i < file->size_blocks; i++) {
        printf("%d ", file->blocks[i]);
    }

    printf("\n");
}

/* write <name> <text>: replace the file's contents with `text`, spreading
 * it across the file's blocks 64 bytes at a time. */
void cmd_write(char *name, char *text) {
    int idx = find_file(name);

    if (idx < 0) {
        printf("Error: file '%s' not found.\n", name);
        return;
    }

    FileEntry *file = &directory[idx];

    /* The file can hold size_blocks * 64 bytes. The check keeps one byte
     * spare, so the largest text accepted is capacity - 1 bytes. */
    int capacity = file->size_blocks * BLOCK_SIZE;
    int len = (int)strlen(text);

    if (len >= capacity) {
        printf("Error: text too large. Capacity is %d bytes.\n", capacity - 1);
        return;
    }

    /* Step 1: erase the old contents of every block the file owns. */
    for (int i = 0; i < file->size_blocks; i++) {
        memset(disk[file->blocks[i]], 0, BLOCK_SIZE);
    }

    int remaining = len;    /* bytes of text still to copy */
    int pos = 0;            /* where in `text` the next chunk starts */

    /* Step 2: copy the text one block at a time, in the order the
     * directory entry lists the blocks. */
    for (int i = 0; i < file->size_blocks && remaining > 0; i++) {
        int block = file->blocks[i];
        int chunk = remaining < BLOCK_SIZE ? remaining : BLOCK_SIZE;   /* a full block, or what is left */

        memcpy(disk[block], text + pos, chunk);

        pos += chunk;
        remaining -= chunk;
    }

    /* Step 3: record the new size in the metadata. */
    file->size_bytes = len;

    printf("Wrote %d byte(s) to '%s'.\n", len, name);
}

/* read <name>: print the file's bytes by visiting its blocks in order. */
void cmd_read(char *name) {
    int idx = find_file(name);

    if (idx < 0) {
        printf("Error: file '%s' not found.\n", name);
        return;
    }

    FileEntry *file = &directory[idx];

    printf("Contents of '%s':\n", name);

    int remaining = file->size_bytes;   /* stop at the file's size, not the block's end */

    for (int i = 0; i < file->size_blocks && remaining > 0; i++) {
        int block = file->blocks[i];
        int chunk = remaining < BLOCK_SIZE ? remaining : BLOCK_SIZE;

        /* fwrite prints exactly `chunk` bytes; the block has no '\0' to stop printf. */
        fwrite(disk[block], 1, chunk, stdout);

        remaining -= chunk;
    }

    printf("\n");
}

/* help: list the commands. */
void cmd_help(void) {
    printf("Commands:\n");
    printf("  create <name> <blocks>   Create a file using N blocks\n");
    printf("  delete <name>            Delete a file\n");
    printf("  ls                       List files\n");
    printf("  bitmap                   Show free-space bitmap\n");
    printf("  stat <name>              Show file metadata\n");
    printf("  write <name> <text>      Write text to file\n");
    printf("  read <name>              Read file contents\n");
    printf("  help                     Show this help\n");
    printf("  quit                     Exit\n");
}

/* fgets keeps the Enter key's '\n' at the end of the line; remove it. */
void trim_newline(char *s) {
    size_t len = strlen(s);

    if (len > 0 && s[len - 1] == '\n') {
        s[len - 1] = '\0';
    }
}

int main(void) {
    char input[INPUT_SIZE];     /* one command line typed by the user */

    init_fs();

    printf("TinyFS Simulator for CMSC421\n");
    printf("Type 'help' for commands.\n\n");

    /* The command loop: print a prompt, read a line, run the command. */
    while (1) {
        printf("fs> ");

        /* fgets returns NULL at end of input (Ctrl-D), which ends the loop. */
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }

        trim_newline(input);

        if (strlen(input) == 0) {   /* the user just pressed Enter */
            continue;
        }

        /* strtok splits the line at spaces. The first call returns the
         * command word; each later strtok(NULL, " ") returns the next word. */
        char *cmd = strtok(input, " ");

        if (!cmd) {     /* the line was only spaces */
            continue;
        }

        /* Match the command word and pull out its arguments. */
        if (strcmp(cmd, "create") == 0) {
            char *name = strtok(NULL, " ");
            char *blocks_str = strtok(NULL, " ");

            if (!name || !blocks_str) {
                printf("Usage: create <name> <blocks>\n");
                continue;
            }

            cmd_create(name, atoi(blocks_str));     /* atoi turns "2" into 2 */
        } else if (strcmp(cmd, "delete") == 0) {
            char *name = strtok(NULL, " ");

            if (!name) {
                printf("Usage: delete <name>\n");
                continue;
            }

            cmd_delete(name);
        } else if (strcmp(cmd, "ls") == 0) {
            cmd_ls();
        } else if (strcmp(cmd, "bitmap") == 0) {
            cmd_bitmap();
        } else if (strcmp(cmd, "stat") == 0) {
            char *name = strtok(NULL, " ");

            if (!name) {
                printf("Usage: stat <name>\n");
                continue;
            }

            cmd_stat(name);
        } else if (strcmp(cmd, "write") == 0) {
            char *name = strtok(NULL, " ");
            /* An empty delimiter list returns the rest of the line,
             * spaces included, so the text can be several words. */
            char *text = strtok(NULL, "");

            if (!name || !text) {
                printf("Usage: write <name> <text>\n");
                continue;
            }

            cmd_write(name, text);
        } else if (strcmp(cmd, "read") == 0) {
            char *name = strtok(NULL, " ");

            if (!name) {
                printf("Usage: read <name>\n");
                continue;
            }

            cmd_read(name);
        } else if (strcmp(cmd, "help") == 0) {
            cmd_help();
        } else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            break;
        } else {
            printf("Unknown command: %s\n", cmd);
            printf("Type 'help' for commands.\n");
        }
    }

    printf("Goodbye.\n");
    return 0;
}
