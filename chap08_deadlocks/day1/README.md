# Chapter 8, Day 1: Deadlock in Real Code

Run on the course Debian 12 VM. Build with `make`.
Slide numbers refer to the Day 1 FA26 v2 deck.

| Slides | Command | File and observation |
|---|---|---|
| 7-11 | `make 01-deadlock` | `two_mutex.c`: opposite lock orders; the runner stops after four seconds. |
| 12-14 | `make 02-livelock` | `livelock.c`: lockstep retries, then random backoff. |
| 15-26 | Return to slides | Four conditions, resource graphs, handling strategies, prevention. |
| 27 | `make 03-prevention` | `two_mutex.c`: one common lock order; both threads finish. |
| 28-29 | `make 04-transfer` | `transfer.c`: source-first ordering, then stable account-id ordering. |
| 30-31 | `make 05-lock-order-check` | `two_mutex.c` with ThreadSanitizer: inspect the serial run for a lock-order inversion. |
| 40 | `make 06-enrichment` | `errorcheck.c`: one thread relocks its own mutex. |

`make run` runs the first four stages without pauses. Run the checker and enrichment separately.

For slides 9-11, run `./two_mutex textbook` in one terminal. Copy its printed PID into these commands in another terminal, replacing 1234:

    pid=1234
    ps -L -o pid,tid,stat,wchan:22,pcpu,comm -p "$pid"
    sudo gdb -q -batch -p "$pid" -ex 'thread apply all bt'

Press Ctrl-C in the first terminal when finished. A timeout or sleeping state alone does not establish deadlock; identify the circular waits.

Read the sanitizer report. A runtime failure does not demonstrate a lock-order inversion.
