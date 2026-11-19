# Chapter 13: File Systems

The file-system lab is in `cmsc421_fs_lab/`. Build and run it on the course VM.

## Demos

| Program | What it shows |
|---|---|
| `random_access_demo.c` | writes `numbers.txt`, then reads single bytes at chosen offsets with `lseek()` |
| `mmap_demo.c` | writes `mapped.txt` through a memory mapping instead of `write()` |
| `allocation_sim.c` | contiguous, linked, and indexed allocation of the same kind of file |
| `fat_trace.c` | follows a file's block chain through a FAT |
| `bitmap_alloc.c` | a free-space bitmap and first-fit allocation of a 4-block file |
| `tinyfs.c` | a small in-memory file system with a prompt: create, write, read, stat, delete, ls, bitmap |

## Build

    cd cmsc421_fs_lab
    make

## Run

    ./random_access_demo
    ./mmap_demo
    ./allocation_sim
    ./fat_trace
    ./bitmap_alloc
    ./tinyfs

`make clean` removes the programs and the files they create.
