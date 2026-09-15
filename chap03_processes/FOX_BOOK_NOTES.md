# Fox, *Linux with Operating System Concepts* — process material on Debian 12

Notes for teaching processes from Richard Fox, *Linux with Operating System Concepts*
(CRC Press, 2014 edition), on the course Debian 12 VM. The book was written against
CentOS 6. Every "Debian 12" statement below was checked on the VM (kernel 6.1, arm64).

Demo numbers without a folder are in this directory (`chap03_processes`).

## Process topics in the book, and where to demo them

| Fox section | Topic | Demos |
|---|---|---|
| 4.1 | process vs program, PID, PPID, init, fork/exec/wait, pipelines | 01, 04, 06, 12 |
| 4.2 | multiprogramming, preemptive multitasking, context switch | 09, 20 |
| 4.2.3 | interrupts, `/proc/interrupts` | 20 |
| 4.3.1 | ownership, real and effective UID, setuid and setgid bits | 02, 15 |
| 4.3.2 | `&`, `jobs`, `fg`, `bg`, Ctrl-Z | 11, 12 |
| 4.4 | `top`, `ps` options and STAT codes, `pstree` | 13, 14 |
| 4.5 | `nice`, `renice`, `pidof`, `lsof` | 13 |
| 4.6.1 | normal and abnormal termination, core dumps, orphans, zombies | 07, 10, 18, 19 |
| 4.6.2 | `kill`, signals, `killall` | 08, 16, 17 |
| 10.5.2 | `/proc/<PID>` files | 21 |

Related sections that belong to other chapters:

| Fox section | Topic | Demo |
|---|---|---|
| 4.2.2, 4.5 | threads vs processes | `chap04_threads/07_threads_vs_processes.c` |
| 4.5 | niceness and CPU share | `chap05_cpu_scheduling/06_nice_cpu_share.c` |
| 14.4.1 | race conditions, deadlock | `chap06_synchronization/06_process_race_and_deadlock.c` |
| 8.6 | virtual memory, running out of memory | `chap10_virtual_memory/02_memory_limits_oom.sh` |
| 8.2 | user and kernel mode, system calls | `Lecture_Material/extra_linux_demos/syscalls_user_kernel.c` |
| 11.4–11.5 | init, runlevels, services | `Lecture_Material/extra_linux_demos/systemd_services.sh` |
| 14.3 | `at`, `batch`, `crontab`, `anacron` | `Lecture_Material/extra_linux_demos/scheduling_cron_at.sh` |

## Corrections for Debian 12

| Fox | The book says | On Debian 12 | Shown in |
|---|---|---|---|
| 4.1 | "The wait system call is like fork except that the parent goes into a wait mode" | `wait` creates nothing. It blocks until a child changes state, then collects (reaps) it. | 04, 07 |
| 4.1, 4.6.1, 4.7 | exec: "the new process inherits the old process' PID"; an exec'd child "takes over for the parent" | `exec` starts no process. The same process, with the same PID and parent, switches to a new program. | 06 (`strace -f`) |
| 4.1 | PIDs "increase by 1", "tens of thousands" | PIDs wrap around and are reused; `pid_max` is 4194304. | 13 |
| 4.3.1 | `passwd` is `-r-s--x--x`; "SUID, GUID" | `/usr/bin/passwd` is `-rwsr-xr-x root root`. The group bit is SGID. | 15 |
| 4.3.2, 4.7 | background processes "run when the CPU has time"; background jobs doing I/O "are stopped" | A background job runs at normal priority. It is only outside the terminal's foreground group; it stops (SIGTTIN) only if it reads from the terminal. | 11 |
| 4.4.1 | load average "0.12 means 12%" | Load average is the average number of runnable (plus uninterruptible) tasks, not a percentage. | 13 |
| 4.4.2, Table 4.2 | STAT codes D R S T W X Z; SZ/VSZ is "swap space size" | Also `t` (stopped by a tracer) and `I` (idle kernel thread: 52 of them in the VM). `W` has meant nothing since Linux 2.6. VSZ is virtual memory size. | 14 |
| 4.5 | "By default, the nice value is +10" | Processes start at nice 0. The `nice` command with no `-n` adds 10. | 13 |
| 4.6.1 | a zombie "cannot be killed until the parent resumes"; "restart the parent" | A zombie is already dead and cannot be killed at all. It disappears when its parent calls `wait`, or when the parent exits and PID 1 reaps it. | 07, 19 |
| 4.6.1 | core dumps may appear in your file space | Core dumps are off by default (`ulimit -c` is 0). | 18 |
| 4.6.2, Table 4.3 | "nine signals available for the kill command"; "SGTRAP"; 6 is SIGIOT | `kill` accepts all 64 (`kill -l`). SIGTRAP is 5; 6 is SIGABRT. The table omits SIGTERM (kill's default), SIGSTOP, SIGTSTP, SIGCONT, and SIGCHLD. | 16 |
| 4.6.3 | `shutdown -c` "forgoes a time unit so that it can shut down immediately" | `shutdown -c` cancels a pending shutdown. With systemd: `systemctl poweroff` or `systemctl reboot`. | — |
| 4.2.3 | `/proc/interrupts` shows an IO-APIC timer counting constantly | The VM shows `arch_timer` on GICv3. The kernel is tickless, so idle CPUs take far fewer timer interrupts. | 20 |

Corrections outside the processes chapter, for whoever teaches those topics:

| Fox | The book says | On Debian 12 | Shown in |
|---|---|---|---|
| 11.4–11.5 | `/etc/inittab`, runlevels, Upstart, `/etc/rc.d/rc5.d`, `service`, `chkconfig`, `telinit` | None of these exist. PID 1 is systemd; `runlevel3.target` and `runlevel5.target` are aliases for `multi-user.target` and `graphical.target`; use `systemctl` and `journalctl`. | `extra_linux_demos/systemd_services.sh` |
| 11.5.2 | crontab entry `0/15/30/45 * 1 1 *` | Separate values with commas: `0,15,30,45 * 1 1 *`. | `extra_linux_demos/scheduling_cron_at.sh` |
| 14.3.1 | `at` and `batch` are available; batch runs when CPU use is under 80% | `at` is not installed by default (`sudo apt install at`). `batch` runs when the load average is under `atd`'s limit (`man atd`, option `-l`). | `extra_linux_demos/scheduling_cron_at.sh` |
| 14.4.1 | Linux applies "the Ostrich algorithm" to deadlocks between user processes | True for `flock()`. For `fcntl()` record locks the kernel detects the cycle and fails a request with `EDEADLK`. | `chap06_synchronization/06_process_race_and_deadlock.c` |

## Process topics the book does not cover

- Process groups, sessions, and the controlling terminal: why Ctrl-C reaches a whole pipeline (02, 11, 12).
- SIGHUP when a terminal closes, and `nohup`, `setsid`, `disown` (17).
- Exit status 128+N and the `waitpid` status macros (16, and `Lecture_Material/L3_demos/L3_06_status`).
- Voluntary versus involuntary context switches (20).
- Child subreapers: why an orphan's PPID is not always 1 (19).
