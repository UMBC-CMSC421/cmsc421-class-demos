#!/bin/bash
# Chapter 10: Virtual Memory
# Demo 02: Memory limits and the out-of-memory (OOM) killer
#
# What this demo shows:
# - A limit on a process's address space (ulimit -v) makes malloc() fail. The program
#   gets an error and can decide what to do.
# - A cgroup memory limit counts memory actually in use. When the group runs out, the
#   kernel's OOM killer sends SIGKILL: exit status 137, with no chance to react.
# - The OOM killer picks its victim by oom_score.
#
# Run:
#   make && ./02_memory_limits_oom.sh
# Needs 01_memory_hog built in this directory. Uses sudo dmesg to show the kernel's log line.

run() {
    echo
    echo "\$ $1"
    eval "$1"
}

if [ ! -x ./01_memory_hog ]; then
    echo "Build the demo first: make"
    exit 1
fi

run "ulimit -v"

echo
echo "--- 1) address-space limit: malloc() fails"
run "(ulimit -v 100000; ./01_memory_hog 200)"

echo
echo "--- 2) cgroup memory limit: the OOM killer ends the process"
run "systemd-run --user --scope --quiet -p MemoryMax=50M -p MemorySwapMax=0 ./01_memory_hog 200; echo \"exit status \$? = 128 + 9 (SIGKILL)\""
run "sudo dmesg | grep -iE 'oom|killed process' | tail -2"

echo
run "cat /proc/self/oom_score /proc/self/oom_score_adj"
echo "  -> the OOM killer picks the process with the highest oom_score; oom_score_adj (-1000..1000) shifts it"
