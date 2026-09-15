# Chapter 6 — Synchronization Tools

These demos focus on races, mutual exclusion, semaphores, condition variables,
and deadlock patterns. They are designed for classroom demonstration and discussion.

## Learning Goals

Students should be able to:

- define a race condition
- explain why mutual exclusion is needed
- use `pthread_mutex_t`
- use semaphores for producer-consumer style coordination
- use condition variables to wait for state changes
- explain one way deadlock can arise

## Demos

1. `01_race_condition.c` — unsynchronized increment race
2. `02_mutex.c` — protect a critical section with a mutex
3. `03_semaphore_producer_consumer.c` — bounded buffer with semaphores
4. `04_condition_variable.c` — wait and signal with a condition variable
5. `05_deadlock_demo.c` — deliberate deadlock from inconsistent lock order
6. `06_process_race_and_deadlock.c` — the same problems between processes sharing a file: lost updates without a lock and none with `flock`; an `fcntl` deadlock the kernel refuses with `EDEADLK`, next to an `flock` deadlock it never detects (Linux only)
