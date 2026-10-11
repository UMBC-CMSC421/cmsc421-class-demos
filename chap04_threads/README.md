# CMSC 421, Chapter 4: Threads

These are small programs you can run to see how threads behave on Linux: how to
create and join them, how they share memory, what goes wrong when two threads
change the same variable, and how a mutex fixes it.

Every `.c` file starts with a comment that covers what the demo is about, how to
run it, and what to look for in the output. Read that part first, then run the
demo. The lines that matter in the code have comments too.

## Before you start

- Use the course Linux VM. Demos 06, 07, and 08 use `gettid()` and `/proc`, which
  only exist on Linux.
- Open two terminals side by side for demo 07. It stops and waits so you can look
  at its threads from the other terminal.
- Run everything from inside this folder.

## Build

```bash
make                          # build the pthread demos (01 to 04, 06 to 08)
make 03_shared_counter_race   # build just one
make openmp                   # build demo 05, which needs OpenMP
make smoke                    # quick check that the pthread demos run
make clean                    # delete the built programs
```

## The demos

| # | Run | What you'll see |
|---|---|---|
| 01 | `./01_pthread_create_join` | one thread created and joined |
| 02 | `./02_thread_args` | four threads, each with its own argument struct, printing in any order |
| 03 | `./03_shared_counter_race` | lost updates: four threads add to one counter and the total comes up short |
| 04 | `./04_mutex_counter_fix` | demo 03 with a mutex; the total is right every time |
| 05 | `./05_openmp_intro` | OpenMP starts a team of threads from one `#pragma` line |
| 06 | `./06_word_count_threads` | two threads each count words in half of a string, and main adds the results |
| 07 | `./07_threads_vs_processes` | threads share one PID and its memory; a forked child gets its own copy (use a second terminal) |
| 08 | `./08_shared_array_no_mutex` | threads sharing an array safely with no mutex, because each writes only its own element |

## Notes

- Run demo 03 several times. The total changes every run.
- Demo 03 must be built with `make`, which turns optimization off. With `gcc -O2`
  the compiler turns the whole loop into one addition and the race almost never
  shows.
- pthread functions return an error number instead of setting `errno`, so these
  demos report errors with `strerror(rc)`, not `perror()`.
- Demo 05 prints one worker per CPU. Try `OMP_NUM_THREADS=2 ./05_openmp_intro`.
- Demo 07 waits for Enter. While it waits, run the `ps -L` command it prints in
  your second terminal.

## Textbook examples

`archive_textbook_examples/` holds example programs from the textbook (`fig4-23.c`,
`thrd-posix.c`, `openmp.c`) and older lecture examples (`pthread1.c`, `threads.c`,
`word_threads.c`). They aren't part of the numbered demos. Build them with
`make archive`.

- `threads.c` names its threads: `./threads & ps -L -o pid,tid,comm -p $!`
- `word_threads.c` is a word processor: type while spell check, autosave, and
  printing run in their own threads. Stop it with Ctrl-C.
