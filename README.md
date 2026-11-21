# CMSC 421: Class Demos

Run these demos on the course Debian 12 Linux VM. Each chapter folder follows
our lecture sequence. Start with its README, then read the comment at the top
of the demo before you build and run it.

## Chapters

| Chapter | Folder | Concepts |
|---|---|---|
| 3: Processes | [chap03_processes/](chap03_processes/) | Process creation, states, signals, IPC, and Linux process inspection |
| 4: Threads | [chap04_threads/](chap04_threads/) | Pthreads, shared memory, mutexes, and OpenMP |
| 5: CPU Scheduling | [chap05_cpu_scheduling/](chap05_cpu_scheduling/) | Dispatching, real-time policies, nice values, and thread scheduling |
| 6: Synchronization | [chap06_synchronization/](chap06_synchronization/) | Races, atomic operations, locks, semaphores, and condition variables |
| 8: Deadlocks | [chap08_deadlocks/](chap08_deadlocks/) | Deadlock, livelock, prevention, avoidance, and detection |
| 9: Main Memory | [chap09_memory_management/](chap09_memory_management/) | Compilation, linking, address binding, and memory-access timing |
| 10: Virtual Memory | [chap10_virtual_memory/](chap10_virtual_memory/) | Memory limits, demand paging, and copy-on-write |
| 13: File Systems | [chap13_file_systems/](chap13_file_systems/) | File access, memory mapping, allocation, and free-space tracking |

## Build and run

Use the Makefile in the chapter or lecture folder. For example:

```sh
cd chap03_processes
make
./04_fork_basics
```

The top-level Makefile prints a reminder to build inside a chapter folder.
Chapter 4's OpenMP demo has a separate target: `make openmp`.
Chapters 6 and 8 have separate Day 1 and Day 2 sequences.

The C demos need GCC and Make. On Debian, install them with:

```sh
sudo apt install build-essential
```

Some demos also use tools such as `strace`, `gdb`, and `lsof`. The chapter README
and source comments list the commands you need. Older Python visual aids use
Manim and are separate from the C builds.

## Work through a demo

1. Read what the program is meant to show.
2. Follow the code from `main` to the functions it calls. For threaded programs,
   identify the shared data and each thread's work.
3. Run the listed command and compare the output with the explanation.
4. Use a second terminal when the demo asks you to inspect a running process.

Some programs deliberately race, deadlock, or fail. Follow their run instructions,
including timeout commands. A correct count from a race example does not prove
that the code is safe. Process IDs, addresses, timing, and thread order can change
between runs.

Folders named `archive_textbook_examples/` hold textbook programs and older
lecture examples. Use the current chapter README for the teaching sequence.
