# Chapter 8, Day 2: Avoidance and Detection

Run on the course Debian 12 VM. Build with `make`.
Slide numbers refer to the Day 2 FA26 v2 deck.

| Slides | Command | File and observation |
|---|---|---|
| 13-20 | `make 01-avoidance` | `bankers.c`: Need, safety trace, then three resource requests. |
| 21-27 | Return to slides | Current waits, wait-for graphs, and the multiple-instance detection trace. |
| 28 | `make 02-detection` | `detect.c`: no deadlock initially; adding T2's request leaves T1 through T4 deadlocked. |
| 29-35 | Return to slides | Detection timing, recovery, and practical systems. |

`make run` runs both programs without a discussion pause.

The banker checks remaining maximum Need. The detector checks current Request.
A safe state can have several safe sequences. Unsafe does not mean already deadlocked.
These programs simulate resource snapshots; they do not recover live mutex deadlocks.
