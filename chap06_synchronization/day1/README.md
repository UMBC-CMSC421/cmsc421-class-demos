# Chapter 6, Day 1: One Counter, Step by Step

Two threads share one counter, and each one adds 1 a million times. You should get **2,000,000**. Every version here keeps that same job and changes how a thread gets permission to touch the counter.

Some versions break on purpose. For each one, ask yourself: what changed, what does it fix, and what can still go wrong?

## Start here

Use the course Linux VM. In this `day1` folder:

```sh
make v1_race      # build the first one
./v1_race         # run it
./v1_race         # run it again: you'll get a different number
```

```sh
make              # build everything
make run          # run them all in lecture order
make clean        # delete the programs
```

Go through them one at a time before you try `make run`. Two things in `make run` fail on purpose: the interrupts demo gets refused, and v2 gets killed after 5 seconds.

## The setup

Open [v1_race.c](v1_race.c) first. Every later version has the same shape: `main` starts two threads, each runs `worker`, and `main` waits for both and prints the count.

`counter++` looks like one step, but it's really three: load the value, add 1, store it back. If two threads both load 5, both store 6, and you just lost an increment.

Every fix fills in the same two blanks:

```c
/* entry section: get permission */
counter++;                          /* critical section */
/* exit section: give it back */
```

And every fix gets graded on three things:

- **Mutual exclusion:** only one thread inside at a time.
- **Progress:** if nobody's inside and someone wants in, somebody gets in.
- **Bounded waiting:** once you ask, only so many others can go ahead of you.

## The slides

Slide numbers are from [CMSC421_Ch6_Day1_FA26.pptx].

| Slides | File | What changes |
|---|---|---|
| 9–19 | [v1_race.c](v1_race.c) | Nothing protects the counter |
| 22–24 | [interrupts_user.c](interrupts_user.c) | Try to turn off interrupts (no counter) |
| 25–28 | [v2_turns.c](v2_turns.c) | Take turns with `turn` |
| 29–33 | [v3_peterson.c](v3_peterson.c) | Peterson's: `flag` plus `turn` |
| 36–39 | [v4_fence.c](v4_fence.c) | Peterson's plus a memory fence |
| 40–42 | [v5_tas.c](v5_tas.c) | Test-and-set spinlock |
| 43–44 | [v5b_cas.c](v5b_cas.c) | Compare-and-swap spinlock |
| 45–46 | [v6_atomic.c](v6_atomic.c) | Atomic counter, no lock |
| 47 | [v7_atomic_limit.c](v7_atomic_limit.c) | Where atomics stop working |

## What you should see

- **v1:** way below 2,000,000, different every time. Waiting for the threads with `pthread_join` isn't the same as protecting the counter.
- **interrupts_user:** "CPU refused" and exit status 1. Only the kernel can turn interrupts off, and even then only on its own CPU.
- **v2:** `./v2_turns 1000000 1000000` gets it right. `timeout 5 ./v2_turns 3 1` hangs: thread 1 finishes, and thread 0 waits forever for a turn nobody's going to give back. Exit status 124 means `timeout` killed it.
- **v3:** run it several times: `for i in 1 2 3 4 5 6 7 8 9 10; do ./v3_peterson; done`. Some runs come up short. It's correct on paper, but real CPUs reorder memory operations. On an emulated VM you might never see it fail.
- **v4:** 2,000,000. The fence keeps my writes in front of my read of your flag.
- **v5 and v5b:** 2,000,000. Only one thread gets the lock, but the others spin and burn CPU, and nobody's guaranteed a turn.
- **v6:** 2,000,000, with no lock at all. Works for one variable.
- **v7:** the last line counts rounds where both consumers took the one item. Each step is atomic, but "check, then take" is two steps. You need a lock around both, and that's Day 2.

Getting 2,000,000 from v1, v3, or v4 doesn't prove they're correct. It just means the bug didn't show up that time.

## Check yourself

1. How can two increments turn 5 into 6 instead of 7?
2. Why doesn't `pthread_join` stop lost updates?
3. Why does `./v2_turns 3 1` get stuck?
4. In Peterson's `while (flag[j] && turn == j)`, what does each half mean?
5. What does the old value from test-and-set tell you?
6. How can a spinlock get the right count but still fail bounded waiting?
7. Why is v6 safe when v7 isn't?

## Try it

Change one thing, rebuild, and see what happens.

- **v1:** drop `N` to 1,000, then 100. Does the bug get harder to see? It never actually becomes safe.
- **v1:** take out `volatile`, rebuild, and run `objdump -d v1_race | less`. Where'd the loop go?
- **v3:** on paper, swap `flag[i] = 1` and `turn = j`. Can you find a way for both threads to get in?
- **v4:** delete the fence and run it a bunch of times. Seeing no difference is a possible result too.
- **v5:** add a third thread and change the expected count to 3,000,000. Why does this lock work for 3 threads when Peterson's doesn't?
- **v7, after Day 2:** put the check and the take inside the same mutex. Why isn't locking just the take enough?
