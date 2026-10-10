# Chapter 6: Synchronization Tools

Run these on the course Linux VM. You'll want at least 2 CPUs (check with `nproc`). They won't build on macOS.

## Start here: the race condition

[01_race_condition.c](01_race_condition.c) is the problem the whole chapter is about. A few threads add 1 to the same counter with no lock, and updates get lost.

```sh
gcc -O2 -Wall -pthread 01_race_condition.c -o race
./race                 # 2 threads
./race 8 20000000      # 8 threads, 20,000,000 adds each
```

`Actual` should come out way below `Expected`, and different every time.

## Then the two lectures

| Folder | What's in it |
|---|---|
| [day1/](day1/) | One counter, fixed step by step: taking turns, Peterson's, a memory fence, test-and-set, compare-and-swap, atomics |
| [day2/](day2/) | The real tools: mutex, spinlock, reader-writer lock, semaphore, condition variable, and deadlock |

Each folder has its own README and Makefile. `make` builds everything, and `make run` runs it all in lecture order. The comment at the top of every file tells you which slides it goes with and what you should see.
