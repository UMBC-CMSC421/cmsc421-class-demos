#!/bin/bash
# Chapter 3: Processes
# Demo 21: /proc/<PID> lets you read a process's PCB as files
#
# What's going on
#   The kernel keeps a record for every process: its state, open files, memory, and
#   so on. The textbook calls it the process control block (PCB). Linux lets you read
#   most of it as files in /proc/<PID>/. They aren't real files on a disk. The kernel
#   writes their contents the moment you read them, which is why ls -l says every one
#   of them is size 0 (Fox 10.5.2).
#   This script starts one sleep process with a known working directory, an extra
#   environment variable, and two extra open files. Then it reads that process's
#   /proc files one by one.
#
# Run
#   ./21_proc_tour.sh
#
# What to look for
#   Everything lines up with how the sleep was started: cwd is /tmp, the environment
#   has CMSC421_DEMO=hello, and file descriptors 3 and 4 point at /etc/hostname and
#   the temp file.
#   At the end, the number of PID directories in /proc and the number of processes ps
#   finds come out about the same. That's because ps gets its list by reading /proc.
#
# Other ways to see it
#   ls /proc/$$                  your shell's own directory ($$ is your shell's PID)
#   cat /proc/self/status        "self" is whoever is reading. Here that's cat, so the
#                                Name: line says cat.
#   ls -l /proc/$$/fd            0, 1, and 2 (stdin, stdout, stderr) point at your terminal
#   man 5 proc                   what every file in there means

scratch=$(mktemp)
cleanup() {
    kill "$subject" 2>/dev/null
    wait 2>/dev/null
    rm -f "$scratch"
}
trap cleanup EXIT

# Print a command with a $ in front, then run it.
run() {
    echo
    echo "\$ $1"
    eval "$1"
}

# The subject. ( ... ) & runs in the background. cd /tmp sets its working directory,
# env adds a variable, 3< and 4> open two extra files, and exec makes sleep take over
# the same process. So $! really is the sleep's PID.
(cd /tmp && exec env CMSC421_DEMO=hello sleep 300 3</etc/hostname 4>"$scratch") &
subject=$!
sleep 0.3
p=/proc/$subject

echo "Subject process: PID $subject, started as: cd /tmp && env CMSC421_DEMO=hello sleep 300 3</etc/hostname 4>$scratch"

run "ls -l $p | head -6"                                                   # size 0 everywhere
run "tr '\\0' ' ' < $p/cmdline; echo"                                      # the command line (NUL-separated, so swap in spaces)
run "tr '\\0' '\\n' < $p/environ | grep CMSC421"                           # the environment variables
run "readlink $p/cwd $p/exe"                                               # working directory, and which program file is running
run "ls -l $p/fd"                                                          # open file descriptors
run "grep -E 'Max (cpu time|open files|processes|core file size)' $p/limits"  # resource limits (what ulimit shows)
run "grep -E '^(Name|State|Pid|PPid|Uid|Threads|VmRSS|voluntary_ctxt)' $p/status"  # the human-readable summary
run "cut -d' ' -f1-5,14,15,18,19,20 $p/stat"                               # the same info in one line, for programs like ps
echo "  -> pid (comm) state ppid pgrp utime stime priority nice num_threads"
run "head -4 $p/maps"                                                      # memory regions (demo 03)
run "cat $p/wchan; echo"
echo "  -> the kernel function the process is sleeping in"
run "grep -E '^(rchar|wchar|syscr|syscw)' $p/io"                           # bytes read/written, read/write system calls
run "head -3 $p/sched"                                                     # scheduler bookkeeping
run "ls $p/task"
echo "  -> one entry per thread; a single-threaded process has just its own PID"
run "ls -d /proc/[0-9]* | wc -l; ps -e --no-headers | wc -l"
echo "  -> ps finds processes by listing /proc, so the counts match (give or take the ls and ps themselves)"
