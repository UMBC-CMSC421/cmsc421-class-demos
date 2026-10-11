#!/bin/bash
# Chapter 3: Processes
# Demo 13: A tour of ps, pstree, pgrep, pidof, lsof, and top
#
# What's going on
#   The script starts a handful of processes, each in a state we already know: one
#   burning CPU, one sleeping, one stopped, one sleeping at low priority, and one
#   with a zombie child. Then it runs the monitoring tools from Fox 4.4 on them, so
#   you can check what each tool says against what you know is true.
#
# Run
#   ./13_monitoring_tour.sh        takes about 5 seconds and cleans up after itself
#
# What to look for
#   - The STAT column: R running, S sleeping, T stopped, N low priority (nice), and
#     Z zombie.
#   - ps options come in two kinds. Some choose WHICH processes to show (-e for all
#     of them, -p, --ppid, -C). Others choose WHICH COLUMNS to show (-o, -f).
#   - The first three numbers in /proc/loadavg are how many tasks were running or
#     waiting to run, averaged over 1, 5, and 15 minutes. They're not percentages.
#
# Try it by hand
#   top           a live view. P sorts by CPU, M sorts by memory, k asks for a PID
#                 to kill, q quits.
#   htop          a friendlier top. F5 switches to a tree view, F10 quits.
#   sleep 600 &                      start something to play with
#   renice -n 5 -p $!                lower its priority ($! is the PID of the last & job)
#   ps -o pid,ni,stat,cmd -p $!      NI is now 5 and STAT has an N in it
#   kill $!                          clean up

# When the script ends, kill everything it started so nothing is left running.
# "${workload[@]}" expands to every PID in the workload array (built below).
# 2>/dev/null throws away error messages, and wait reaps the killed processes.
cleanup() {
    kill -KILL "${workload[@]}" 2>/dev/null
    wait 2>/dev/null
}
# trap ... EXIT tells bash to call cleanup when the script exits.
trap cleanup EXIT

# Print a command with a $ in front, then run it. $1 is the text passed to run.
# eval runs that text as a command, so the pipes and quotes inside it work.
run() {
    echo
    echo "\$ $1"
    eval "$1"
}

# The workload: one process in each common state. $! is the PID of the last & job.
# workload=() starts an empty array, and workload+=(...) adds a PID to the end of it.
workload=()
sh -c 'while :; do :; done' &                  workload+=($!)   # R  burns CPU
sleep 600 &                                     workload+=($!)   # S  sleeping
sleep 601 & stopped=$!                                           # T  stopped (wait until the
sleep 0.2; kill -STOP "$stopped";               workload+=("$stopped")   # child has exec'd sleep)
nice -n 10 sleep 602 &                          workload+=($!)   # SN sleeping, low priority
bash -c 'sleep 0.1 & exec sleep 603' &          workload+=($!)   # its child becomes a zombie:
                                                                  # "sleep 603" never calls wait()
# Give every process a second to settle into its state before we look.
sleep 1
pids=$(IFS=,; echo "${workload[*]}")             # the PIDs as one comma-separated list

echo "Workload PIDs: ${workload[*]}   (this script is PID $$)"

# The tour. Each line prints a command, runs it, and the comment says what it shows.
# $$ is this script's PID, so --ppid $$ means "the processes this script started".
run "ps"                                                          # processes on this terminal
run "ps -f --ppid $$"                                             # -f: full format. --ppid $$: this script's children
run "ps -o pid,ppid,ni,pri,stat,%cpu,rss,etime,cmd --ppid $$"     # -o: choose your own columns
run "ps -eo pid,ppid,stat,cmd | awk 'NR == 1 || \$3 ~ /^Z/'"      # every process, keep only zombies
run "ps aux --sort=-%cpu | head -4"                               # the classic "ps aux", top CPU users first
run "ps -e --forest -o pid,stat,cmd | grep -A8 -E '^ *$$ '"       # --forest draws parent/child as a tree
run "pstree -p $$"                                                # the same tree, drawn nicer
run "pgrep -a sleep"                                              # find PIDs by name (-a adds the command line)
run "pidof sleep"                                                 # same idea, PIDs only
run "lsof -p ${workload[1]} | head -8"                            # files the sleeping process has open
run "top -b -n 1 -p $pids | tail -7"                              # one snapshot of top (-b: print it, don't go interactive)
# /proc/loadavg is a file the kernel fills in every time you read it.
run "cat /proc/loadavg"
echo "  -> 1-, 5- and 15-minute averages of runnable tasks, then running/total tasks and the last PID."
echo "     The busy loop pushes the 1-minute figure toward 1.0 if you run this for a minute."
