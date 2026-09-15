# CMSC 421, Chapter 3: Processes

These are small programs you can run to see how processes really behave on Linux:
how they get created, what IDs and memory they have, what states they go through,
how signals work, and how to watch all of that from the outside.

Every `.c` and `.sh` file starts with a comment that covers four things: what the
demo is about, how to run it, what to look for in the output, and other ways to
see the same thing. Read that part first, then run the demo. The lines that matter
in the code have comments too.

## Before you start

- Use the course Linux VM. Demos 01 to 10 also build on a Mac, but 11 to 21 only
  work on Linux, and most of the tools in the "other ways to see it" sections
  (`/proc`, `strace`, GNU `ps`) are Linux only.
- Open two terminals side by side. A lot of these demos have you run the program
  in one terminal and look at it from the other.
- Run everything from inside this folder.
- On your own Debian or Ubuntu machine, install anything that says "command not
  found":
  `sudo apt install build-essential strace gdb psmisc lsof sysstat time`

## Build

```bash
make                  # build every demo
make 04_fork_basics   # build just one
make smoke            # quick check that 01-10 run (skips 07 and 08, which take a while)
make smoke-linux      # quick check that 12-21 run (Linux only, about 20 seconds)
make clean            # delete the built programs
```

## Part 1: the basics (01 to 10)

| # | Run | What you'll see |
|---|---|---|
| 01 | `./01_proc_identity` | a process's PID and its parent's PID |
| 02 | `./02_process_attributes` | process group, session, and user IDs |
| 03 | `./03_memory_layout` | where globals, the heap, and the stack sit in memory |
| 04 | `./04_fork_basics` | `fork()` returning twice, with a different value each time |
| 05 | `./05_fork_cow` | parent and child changing "the same" variable separately |
| 06 | `./06_fork_exec` | a child turning into a different program with `exec` |
| 07 | `./07_zombie_demo` | a zombie process for 10 seconds (use a second terminal) |
| 08 | `./08_signals` | catching Ctrl-C and `SIGUSR1` (use a second terminal) |
| 09 | `./09_context_switch_pressure 500` | what it costs to create lots of processes |
| 10 | `./10_process_state_lifecycle` | one child going from stopped, to sleeping, to zombie, to gone |

## Part 2: Linux deep dive (11 to 21)

These follow the process chapter of Richard Fox, *Linux with Operating System
Concepts* (mostly chapter 4). The Fox column is the section in the book.

| # | Run | Fox | What you'll see |
|---|---|---|---|
| 11 | `./11_foreground_background` | 4.3.2 | Ctrl-Z, `jobs`, `bg`, and `fg` (needs a real terminal) |
| 12 | `./12_pipeline_process_group.sh` | 4.1 | a pipeline is three processes in one process group |
| 13 | `./13_monitoring_tour.sh` | 4.4 | `ps`, `pstree`, `pgrep`, `pidof`, `lsof`, and `top` |
| 14 | `./14_state_codes` | 4.4.2 | child processes in the R, S, D, T, t, and Z states |
| 15 | `./15_setuid_identity` | 4.3.1 | real vs effective user ID (the full version needs `sudo`) |
| 16 | `./16_signal_dispositions` | 4.6.2 | what 22 signals do by default, and exit status 128+N |
| 17 | `./17_terminal_hangup.sh` | 4.6.2 | closing a terminal kills its jobs, and how to keep one alive |
| 18 | `./18_core_dump` | 4.6.1 | a segfault, a core file, and finding the bug with gdb |
| 19 | `./19_orphan_subreaper` | 4.6.1 | who adopts a process after its parent dies |
| 20 | `./20_context_switches` | 4.2.2 | voluntary vs involuntary context switches |
| 21 | `./21_proc_tour.sh` | 10.5.2 | reading a process's PCB out of `/proc/<PID>` |

The book was written for CentOS 6, so some of it is out of date on Debian 12.
`FOX_BOOK_NOTES.md` lists those spots and which demo shows the difference.

## Watching any process

These work on any demo that stays alive long enough to look at (07, 08, 11, 14,
or `./03_memory_layout --hold`). They also work on your own shell, whose PID is `$$`.

```bash
ps -o pid,ppid,pgid,sid,stat,ni,%cpu,rss,cmd --forest   # your processes, as a tree
ps -o pid,ppid,stat,cmd -C 07_zombie_demo                # find processes by program name
pstree -p $$                                             # your shell and everything under it
cat /proc/<PID>/status                                   # state, IDs, memory, context switches
cat /proc/<PID>/maps                                     # memory regions
ls -l /proc/<PID>/fd                                     # open files
strace -f ./06_fork_exec                                 # every system call, children included
top                                                      # live view (htop is nicer), q quits
```

The STAT column in `ps`:

- First letter: `R` running, `S` sleeping, `D` sleeping and can't be interrupted,
  `T` stopped, `t` stopped by a debugger, `Z` zombie.
- Extra letters after it: `+` in the terminal's foreground group, `s` session
  leader, `N` low priority, `<` high priority, `l` has multiple threads.

## If something gets stuck

- Ctrl-C ends most demos. `08_signals` catches Ctrl-C on purpose, so press it
  three times, or press Ctrl-\ to quit it right away.
- A job you stopped with Ctrl-Z is still there. `jobs` lists them, `fg` brings one
  back, and `kill %1` ends job number 1.
- Leftover processes from a demo: `pkill <program name>`, for example
  `pkill 14_state_codes`. `ps -u $USER` shows everything you're running.

## Other things in this folder

- `FOX_BOOK_NOTES.md`: where the Fox book and Debian 12 disagree.
- `archive_textbook_examples/`: the original sample code from the textbook. It's
  kept for reference, but the Makefile doesn't build it. Some of it is incomplete
  or Windows only. Use the numbered demos instead.

Related demos in other folders: `chap04_threads/07_threads_vs_processes.c`,
`chap05_cpu_scheduling/06_nice_cpu_share.c`,
`chap06_synchronization/06_process_race_and_deadlock.c`, and
`chap10_virtual_memory/02_memory_limits_oom.sh`. Topics that don't have a chapter
folder yet (system calls, systemd, cron and at, namespaces) are in
`Lecture_Material/extra_linux_demos/`.
