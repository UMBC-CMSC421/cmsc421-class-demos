# Chapter 3 — Processes

This folder contains Linux C demos for core process concepts from the textbook.

## Learning Goals

Students should be able to:

- identify a process by PID and parent PID
- explain the basic memory regions of a program
- describe what `fork()` does
- explain why parent and child can run in different orders
- describe `exec()` as program replacement
- explain zombie processes
- observe signal delivery and handling

## Demos

1. `01_proc_identity.c` — PID, PPID, argv, and environment basics
2. `02_memory_layout.c` — stack, heap, global, static, and text-region observations
3. `03_fork_basics.c` — basic `fork()` behavior and nondeterministic scheduling
4. `04_fork_cow.c` — copy-on-write intuition through separate parent/child updates
5. `05_fork_exec.c` — `fork()` followed by `exec()` and `waitpid()`
6. `06_zombie_demo.c` — creating and observing a zombie process
7. `07_signals.c` — signal handling with `SIGINT` and `SIGUSR1`
8. `08_context_switch_pressure.c` — many short-lived workers to discuss context-switch pressure

## Build

```bash
make
```

## Run Examples

```bash
./01_proc_identity hello world
./03_fork_basics
./05_fork_exec
./06_zombie_demo
```

## Tools to Use Alongside the Demos

```bash
ps -o pid,ppid,stat,cmd
pstree -p
cat /proc/<PID>/status
pmap <PID>
strace -f ./05_fork_exec
```

## Instructor Notes

For `06_zombie_demo.c`, run `ps -o pid,ppid,stat,cmd` in another terminal while the parent sleeps.
Students should see the child in `Z` state until the parent calls `waitpid()`.
