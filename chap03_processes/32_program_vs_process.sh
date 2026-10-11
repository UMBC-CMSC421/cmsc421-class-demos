#!/bin/bash
# Chapter 3: Processes
# Demo 32: A program is a file on disk; a process is that file running
#
# What's going on
#   /usr/bin/sleep is a program: an executable file that just sits on disk. Nothing
#   is running, so it has no PID. When you start it, the kernel loads that file into
#   memory and creates a process with its own PID, state, and parent. Start it twice
#   and you get two processes from the same one file.
#   We use sleep, not ls, because ls finishes in milliseconds. sleep 300 waits five
#   minutes, so there is time to look at it.
#
# Run
#   ./32_program_vs_process.sh
#
# What to look for
#   Step 1: ls -l shows a file: size, owner, permissions. The x bits mean executable.
#   Step 2: ps shows two sleep lines with two different PIDs and the same PPID
#           (this script). STAT S means sleeping, which is what sleep does.
#   Step 3: both processes point back to the same file, /usr/bin/sleep.
#   Step 4: killing one process leaves the other one running and the file untouched.
#
# Other ways to see it
#   sleep 300 &  then  ps -o pid,ppid,stat,cmd      the same thing by hand
#   kill %1                                          stop the first background job
#   pgrep -a sleep                                   every sleep process on the machine

# When the script ends (trap ... EXIT), stop any sleep we started and reap it.
cleanup() {
    kill "$first" "$second" 2>/dev/null
    wait 2>/dev/null
}
trap cleanup EXIT

# Print a command with a $ in front, then run it. eval runs a string as a command,
# so pipes inside it work like you typed them.
run() {
    echo
    echo "\$ $1"
    eval "$1"
}

echo "Machine: $(uname -m), kernel $(uname -r)"

echo
echo "== Step 1: the program, a file on disk =="
run "ls -l /usr/bin/sleep"

echo
echo "== Step 2: the process, that file running (twice) =="
# & runs the command in the background, so the script keeps going.
# $! is the PID of the job we just started.
echo
echo "\$ sleep 300 &"
sleep 300 &
first=$!
echo "[1] $first"
echo "\$ sleep 300 &"
sleep 300 &
second=$!
echo "[2] $second"
sleep 0.2          # give both a moment to start
run "ps -o pid,ppid,stat,cmd"

echo
echo "== Step 3: each process points back to the file it came from =="
run "readlink /proc/$first/exe /proc/$second/exe"

echo
echo "== Step 4: kill one process; the other process and the file are untouched =="
run "kill $first; wait $first 2>/dev/null"
run "ps -o pid,ppid,stat,cmd -p $first,$second"
run "ls -l /usr/bin/sleep"
