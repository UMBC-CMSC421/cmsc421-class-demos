# CMSC 421, Chapter 5: CPU Scheduling

These programs schedule real processes on Linux, so you can watch the policies
from the slides happen instead of working them out on paper. Demos 01 and 02 run
the workloads from the slides and print a measured timeline and the turnaround,
waiting, and response times, so you can check them against the scorecard.

Every `.c` file starts with a comment that covers what the demo is about, how to
run it, and what to look for in the output. Read that part first, then run the
demo. The lines that matter in the code have comments too.

## Before you start

- Use the course Linux VM. These demos use Linux-only calls such as
  `sched_setaffinity()` and real-time scheduling policies.
- Demo 02 and the second run of demo 04 need `sudo`, because real-time
  policies need root.
- Open two terminals side by side. Several demos suggest a `ps` command to run
  from the other terminal while they run.
- Run everything from inside this folder.

## Build

```bash
make                  # build every demo
make 01_dispatcher    # build just one
make smoke            # quick check of the demos that don't need sudo (under 10 seconds)
make clean            # delete the built programs
```

## The demos

| # | Run | What you'll see |
|---|---|---|
| 01 | `./01_dispatcher fcfs` (also `sjf`, `srtf`, `rr 4`, and `-s` for slow) | the parent schedules four real processes with SIGSTOP and SIGCONT; the average waits match the slides |
| 02 | `sudo ./02_kernel_sched fifo` (also `rr`, `prio`) | the kernel schedules the same four with SCHED_FIFO and SCHED_RR, and the priority slide's five jobs with real-time priorities |
| 03 | `./03_nice_cpu_share` | two processes sharing one CPU at nice 0 and nice 5: about 75% and 25%, as on the nice weights slide |
| 04 | `./04_pthread_sched_attr`, then `sudo ./04_pthread_sched_attr` | asking Pthreads for SCHED_FIFO: why it's ignored without PTHREAD_EXPLICIT_SCHED, and why it fails without root |

## Notes

- Linux has no SJF or SRTF policy, because the kernel can't know how long a CPU
  burst will be. Demo 01 is told the bursts in advance, which is what SJF assumes.
- Linux real-time priorities run from 1 to 99, and a higher number wins. The
  textbook uses lower number = higher priority.
- In demo 02, the kernel gives real-time tasks only 950 ms of every second. The
  50 ms gaps show up in the timeline and push the averages a little above the
  hand-calculated values. The comment at the top of `02_kernel_sched.c` shows
  how to turn that off and back on.
- The course VM runs Linux 6.1, whose normal scheduler is CFS. Kernels 6.6 and
  later use EEVDF instead. `uname -r` shows your version.

## Textbook examples

`archive_textbook_examples/` holds two programs from the textbook: `posix-sched.c`
(contention scope) and `posix-rt.c` (scheduling policy). They aren't part of the
numbered demos. Demo 04 covers the same calls. Build them with `make archive`.
