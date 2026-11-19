# Chapter 8: Deadlocks

Run these demos on the course Debian 12 VM.

Build the current lecture demos with `make`.

| Folder | Sequence | Run |
|---|---|---|
| day1/ | Deadlock, livelock, prevention, transfer lock ordering | `make run-day1` |
| day2/ | Banker's avoidance algorithm, then deadlock detection | `make run-day2` |

Use the numbered targets in each day's README to stop at the matching slides.
ThreadSanitizer and self-relock enrichment have separate Day 1 targets.

Supplemental files:

- `01_deadlock_conditions.md`: four necessary conditions.
- `02_deadlock_detection_sim.c`: single-instance wait-for graph simulation.
- `03_bankers_algorithm_sim.c`: standalone safety-check simulation.

Build the two supplemental simulations with `make legacy`.
