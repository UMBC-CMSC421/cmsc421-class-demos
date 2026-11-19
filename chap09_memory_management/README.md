# Chapter 9: Main Memory

## Purpose
Follow a C program from source code to a running process, and see what memory accesses cost.

## Demos
1. `01_compile_steps/`: two C files and a header, built one step at a time (preprocess, compile, assemble, link, load, run). `./run_steps.sh` pauses before each step; `make preprocess`, `make compile`, `make assemble`, `make link`, and `make run` run one step each.
2. `02_binding/`: Day 1 address-binding demos.
   - `prog1.c`: one global and one library call; `nm` shows T, B, and U symbols before and after linking.
   - `where.c`: prints the run-time addresses of `main` and a global; two runs differ because of ASLR.
   - `cost.c`: times a cache hit, a RAM access, and a kernel round trip (`getpid`).

`fragmentation.py` and `paging.py` are older visual aids for this chapter.

## Build
```
cd 01_compile_steps && make     # or ./run_steps.sh
cd 02_binding && make
```

## Run
```
./prog1
./where ; ./where
./cost
```

## What Students Should Observe
- `nm` shows `U` for names defined in another file or library, and offsets, not RAM addresses, after linking.
- `where` prints a different address for `main` on every run, but the linker's offset stays the same.
- A kernel round trip costs as much as many cache hits.
