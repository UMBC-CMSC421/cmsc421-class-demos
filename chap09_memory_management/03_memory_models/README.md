# Chapter 9: Trace an Address Through Memory

Use the course Linux VM. Each program follows one mechanism from Chapter 9.
Run it once, then use `--step` to pause before each result and work through the code.

```sh
make
make check
./01_base_limit --step
```

Run commands from this folder. `make run` runs all six programs without pauses.

| # | Run | What you'll see |
|---|---|---|
| 01 | `./01_base_limit` | Check the limit before adding the base; compare address 999 with 1000, then relocate the process. |
| 02 | `./02_allocation` | Compare first, best, and worst fit from the same memory map; free blocks, compact memory, and merge adjacent holes. |
| 03 | `./03_page_translation` | Follow logical address 13 to the letter n; compare page boundaries, a second process, hexadecimal translation, and unused space in the last frame. |
| 04 | `./04_tlb` | Follow a six-access trace through a two-entry TLB; count hits and misses, then switch processes using tagged entries. |
| 05 | `./05_page_protection` | Ask Linux to change page permissions, then observe successful reads and writes and deliberate SIGSEGV signals in child processes. |
| 06 | `./06_two_level` | Split a 32-bit address into two indexes and an offset; compare populated and missing entries and calculate table storage. |

## Change the address

Demos 01, 03, and 06 accept one decimal or hexadecimal address:

```sh
./01_base_limit 999
./01_base_limit 1000
./03_page_translation 13
./03_page_translation 16
./06_two_level 0x00403ABC
./06_two_level 0x00404000
```

You can combine an address with `--step`. Negative numbers, values above
4294967295, and extra arguments are rejected. A simulated invalid access prints
a trap or stop message and completes normally.

## What each model represents

Demos 01 to 04 and 06 are textbook simulations. Their addresses are numbers in
a model, not physical addresses read from the Linux page tables. The allocation
model is contiguous process allocation, not the implementation of `malloc()`.

The TLB model uses a two-entry FIFO replacement rule, process tags, and a
one-level page table. All valid pages are resident. Its access counts and
15 ns example are calculated from that model, not measured CPU timings.

Demo 05 exercises real Linux page permissions with `mmap()` and `mprotect()`.
The parent reports each child's exit status and checks the expected result.
Core dumps are disabled for the deliberate faults. A protection fault is not
evidence that data must be read from disk.

Demo 06 uses the textbook's 10/10/12 split, with 4-byte entries and 4 KiB pages.
The course VM runs x86-64; this simulation does not claim to inspect its page-table
layout. Its storage calculation excludes data frames.

## Follow the code

Start in `main()`. Each call advances the example, and `pause_step()` pauses
before the next result. Comments explain the checks, lookups, and state changes.
`walkthrough.h` handles command-line input and pauses. It does not perform the
memory-management algorithms.

The examples follow Chapter 9, Sections 9.1 to 9.4. Linux page-permission behavior
is documented in `man 2 mprotect` and `man 2 mmap`.
