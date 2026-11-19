
# Chapter 10: Virtual Memory

## Purpose
Demonstrate core operating system concepts from this chapter.

## Demos
1. `01_memory_hog.c`: allocates and touches memory 10 MB at a time until something stops it
2. `02_memory_limits_oom.sh`: an address-space limit makes `malloc()` fail; a cgroup memory limit gets the process OOM-killed (exit status 137). Linux only; uses `sudo dmesg`
3. `03_lazy_alloc.c`: maps 4 GiB, touches one page, then 256 MiB; prints virtual size, resident size, and fault counts after each step. Asks for 4 KiB pages with `madvise(MADV_NOHUGEPAGE)`, so it shows one minor fault per page even though the course VM sets transparent huge pages to `always`
4. `04_cow_fork.c`: after `fork()`, the child reads, then writes, a 64 MiB buffer; counts the copy-on-write faults

## Build
make

## Run
./02_memory_limits_oom.sh
./03_lazy_alloc
./04_cow_fork

## What Students Should Observe
- System behavior
- Process or thread activity
- OS interactions
