# Chapter 5 — CPU Scheduling

Real kernels make scheduling decisions in privileged code, so this chapter uses
**simulation programs** to make the policies visible. These demos are ideal for lecture,
recitation, or activity support.

## Learning Goals

Students should be able to:

- read a process table with arrival and burst times
- simulate FCFS, SJF, SRTF, and Round Robin
- compute completion, turnaround, and waiting times
- explain starvation and fairness at a high level
- connect textbook policy ideas to real system schedulers

## Demos

1. `01_fcfs_sim.c` — First-Come, First-Served
2. `02_sjf_sim.c` — Shortest Job First (non-preemptive)
3. `03_rr_sim.c` — Round Robin with configurable quantum
4. `04_priority_sim.c` — Priority scheduling (non-preemptive)
5. `05_posix_sched_demo.c` — inspect scheduler policy for the current process

## Build

```bash
make
```
