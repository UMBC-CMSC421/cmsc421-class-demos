# Chapter 4 — Threads

This folder contains Linux C demos for thread creation, shared memory, race conditions,
and basic synchronization using POSIX threads.

## Learning Goals

Students should be able to:

- create a thread using `pthread_create()`
- pass arguments to a thread function
- explain why threads share an address space
- observe a race condition
- use a mutex to enforce mutual exclusion
- compare POSIX threads and OpenMP at a high level

## Demos

1. `01_pthread_create_join.c` — create one thread and join it
2. `02_thread_args.c` — pass structured data into threads
3. `03_shared_counter_race.c` — observe lost updates without synchronization
4. `04_mutex_counter_fix.c` — repair the race with a mutex
5. `05_openmp_intro.c` — optional OpenMP hello demo
6. `06_word_count_threads.c` — mini-program showing work splitting and joins

## Build

```bash
make
```

## Notes

Run the race-condition demo more than once. The result may vary.
For OpenMP, make sure your gcc supports `-fopenmp`.
