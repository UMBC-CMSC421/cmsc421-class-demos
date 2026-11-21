# CMSC 421, Chapter 9: Main Memory

Follow a C program from source code to a running process. Then compare the
cost of a cached memory access, a large random memory access, and a system call.
Use the course Linux VM.

## Compilation and linking

`01_compile_steps/` contains two C files and a header. Build and run them from
that folder:

```sh
cd 01_compile_steps
make
./run_steps.sh
```

The script pauses before each step: preprocess, compile, assemble, link, load,
and run. Use `./run_steps.sh -n` to run without pauses. The Makefile also has
`preprocess`, `compile`, `assemble`, `link`, and `run` targets.

Follow the call to `greet()`. Its declaration comes from the header, its definition
is compiled into `greet.o`, and the linker connects the call in `main.o` to it.

## Address binding and memory cost

From the chapter folder:

```sh
cd 02_binding
make
./prog1
./where
./where
./cost
```

If you are still in `01_compile_steps/`, use `cd ../02_binding` instead.

| File | What to follow |
|---|---|
| `prog1.c` | Use `nm` to compare defined and undefined symbols before and after linking. |
| `where.c` | Compare the addresses printed by two runs with the symbol values from `nm`. |
| `cost.c` | Compare a 16 KiB pointer chain, a 512 MiB random pointer chain, and `syscall(SYS_getpid)`. The program needs about 1 GiB of free memory. |

With the course VM's position-independent executable build and ASLR enabled,
run-time addresses can change between runs. The symbol values in the executable
stay fixed. Follow the commands in `where.c` to inspect both.

The timing results depend on the machine and workload. Use them to compare the
three operations on the same machine, not as fixed memory or system-call costs.

## Memory walkthroughs

`03_memory_models/` contains six demos: base and limit, contiguous allocation,
page-table translation, a TLB trace, Linux page protection, and two-level page
tables. See [the walkthrough README](03_memory_models/README.md) for the sequence.

From the chapter folder, build all C demos and check the new examples with:

```sh
make
make check
```

Use `--step` on any new demo to pause before each result. Five demos simulate
textbook mechanisms; the page-protection demo exercises the real Linux VM.

## Python visual aids

`fragmentation.py` and `paging.py` are older Manim scenes, separate from these C
demos. The paging scene follows logical address 13 through page 3 to frame 2,
keeping offset 1, which gives physical address 9.
