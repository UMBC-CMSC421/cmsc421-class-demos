# Chapter 6, Day 2: The Tools

On Day 1 you fixed one counter using only what the hardware gives you. Today it's the real tools, the ones you'll use in Project 2: mutexes, spinlocks, reader-writer locks, semaphores, and condition variables. Then you'll see how locks that work fine can still get stuck.

Every program here goes with a slide, and most have an "in Code" slide that walks through it. They're listed in lecture order.

## Build and run

Use the course VM (you need at least 2 CPUs; check with `nproc`). These won't build on macOS.

```
make          # build everything
make run      # run everything in lecture order (takes under a minute)
make clean    # delete the programs
```

Or build one by hand:

```
gcc -O2 -Wall -pthread v8_mutex.c -o v8_mutex
./v8_mutex
```

Two of them hang on purpose: `./deadlock opposite` and `./dining left`. Hit Ctrl-C to stop them. `make run` kills them after 4 seconds.

## The demos

| File | Slides | Run | What you should see | The point |
|---|---|---|---|---|
| `v8_mutex.c` | Version 8: The Counter with a Mutex | `./v8_mutex` | 2,000,000 every time | Day 1's v1 plus three lines: a mutex, lock, and unlock. |
| | How Linux Builds pthread_mutex_lock | `strace -f -c -e trace=futex ./v8_mutex 1`, then `2` | 1 futex call with 1 thread, hundreds or thousands with 2 | A free lock never goes to the kernel. That 1 call is `pthread_join`. |
| `nested.c` | Nesting Two Locks in Code | `./nested` | The checker sees a + b = 200 every time, and it ends at a = 100, b = 100. | One update, two locks. Everybody locks a, then b, and unlocks in reverse. |
| `spin_vs_sleep.c` | Spinlock or Sleeping Lock?; Spinlock vs. Mutex in Code | `spin 2`, then `mutex 2`; then both with `$((4 * $(nproc)))` | 2 threads: spin is about as fast or faster. 4 threads per CPU: the mutex wins and uses way less CPU. | A spinning thread burns its CPU. A sleeping one hands it over. |
| `bank.c` | Your Turn: Lock the Account; The Bank in Code | `./bank none`, `inside`, `whole` | none and inside: both customers get paid in some rounds. whole: 0 and 0. | The check and the withdrawal have to be one critical section. |
| `rwlock.c` | A Reader-Writer Lock: What It Does; Does a Reader-Writer Lock Meet Them?; ...in Code | `./rwlock mutex`, `rw`, then `writer` | mutex: 1 reader at a time, slowest. rw: 4 at once, about 3x faster, but the writer waits hundreds of ms. writer: 4 at once, and the writer waits only a few ms. Rule breaks: always 0. | Lots of readers at once, or one writer, never both. The default lock favors readers, so the writer can get stuck waiting. |
| `sem_permits.c` | A Semaphore: What It Does; ...in Code | `./sem_permits`, then `./sem_permits 1` | T1 and T2 get permits. T3 waits until T1 gives one back at 1.0 s. With 1 permit, it's a lock. | wait() takes a permit or sleeps. signal() gives one back and wakes someone. |
| `sem_order.c` | Three Ways to Use a Semaphore; Ordering with a Semaphore in Code | `./sem_order nosem`, then `sem` | nosem: S2 prints first. sem: S1 always prints first. | A semaphore that starts at 0 makes S2 wait for S1. |
| `sem_barge.c` | Does a Semaphore Meet the Requirements?; How Linux Builds sem_wait and sem_post | `./sem_barge posix`, then `textbook` | posix: the busy thread cuts in line several times per wait, and tens to thousands at worst. textbook: at most once. | Linux's semaphore wakes you up but doesn't save the permit for you, so there's no bounded waiting. The textbook's version hands it straight to you. |
| `condvar.c` | A Condition Variable: What It Does; Condition Variables in Code | `./condvar while`, then `if` | while: the second consumer goes back to sleep, and it ends with "no". if: it takes an item that isn't there, and it ends with "YES (bug)". | Always recheck in a while loop after you wake up. |
| `deadlock.c` | Deadlock; Deadlock in Code | `./deadlock opposite` (Ctrl-C), then `ordered` | opposite: each thread holds one lock and waits forever. ordered: both finish. | Everybody locks in the same order, and this can't happen. |
| `dining.c` | Dining Philosophers with Semaphores; ...in Code | `./dining left` (Ctrl-C), then `asymmetric` | left: five "has chopstick" lines, then nothing. asymmetric: everyone eats 3 meals. | Everyone grabs left: stuck. Odd seats left first, even seats right first: no cycle. |
| `cas_vs_mutex.c` | Evaluating the Tools: Contention; Optimistic vs. Pessimistic in Code | `for t in 1 2 4 8; do ./cas_vs_mutex cas $t; ./cas_vs_mutex mutex $t; done` | Failed CAS count goes up as you add threads. | More threads, more collisions, more retries. |

Your numbers won't match anyone else's exactly. That's normal.

## Practice

1. **v8.** Delete the lock and unlock. Do you get v1's behavior back? Now move them outside the `for` loop. Still right? What happened to running in parallel?
2. **v8.** Try `strace -f -c -e trace=futex ./v8_mutex 4`. Why more futex calls than with 2?
3. **spin_vs_sleep.** Find the thread count where the mutex starts winning on your machine. How does it compare to `nproc`?
4. **bank.** Add a `deposit(50)` thread to each round. Where does its lock go? What balances can you end up with?
5. **rwlock.** Set `WRITES` to 200 and take out the writer's `usleep(5000)`. Does the reader-writer lock still win?
6. **sem_permits.** Before you run `./sem_permits 3`, guess when T4 and T5 get permits.
7. **sem_order.** Add a P3 whose S3 has to run after S2. How many semaphores, and what do they start at?
8. **condvar.** In `if` mode, switch `pthread_cond_broadcast` to `pthread_cond_signal`. Does the bug go away? Is it actually correct now? (Hint: spurious wakeups.)
9. **deadlock.** Swap `pthread_mutex_lock(second)` for `pthread_mutex_trylock(second)`. If it fails, unlock `first`, wait a bit, and try again. Still stuck? What new problem did you just make?
10. **dining.** Fix `left` mode another way: only let four philosophers try at once, with a semaphore that starts at 4.

## Good to know

- **Semaphore values on Linux.** In the textbook, the value can go negative, and that number tells you how many are waiting. Linux just stops at 0. Same behavior, different bookkeeping.
- **No priority inversion demo.** You'd need real-time scheduling, which needs root and a carefully set-up VM.
- **The delays are on purpose.** `bank.c`, `deadlock.c`, and `dining.c` pause in just the right spot so the bug shows up every run (or close to it). The bugs are there without the pauses; you'd just see them less.
