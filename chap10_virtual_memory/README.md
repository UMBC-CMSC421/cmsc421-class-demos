
# Chapter 09 — Virtual Memory

## Purpose
Demonstrate core operating system concepts from this chapter.

## Demos
1. `01_memory_hog.c` — allocates and touches memory 10 MB at a time until something stops it
2. `02_memory_limits_oom.sh` — an address-space limit makes `malloc()` fail; a cgroup memory limit gets the process OOM-killed (exit status 137). Linux only; uses `sudo dmesg`

## Build
make

## Run
./02_memory_limits_oom.sh

## What Students Should Observe
- System behavior
- Process or thread activity
- OS interactions
