#!/bin/bash
# Chapter 3: Processes
# Demo 17: Closing a terminal sends SIGHUP to its jobs
#
# What's going on
#   SIGHUP ("hang up", signal 1) is what a shell's jobs get when their terminal goes
#   away: you close the terminal window, or your SSH connection drops (Fox 4.6.2).
#   Background jobs, running or stopped, die along with the terminal, unless you
#   started them one of these ways:
#     nohup cmd &      the job ignores SIGHUP
#     setsid cmd &     the job starts in a brand new session that has no terminal
#     cmd & disown     the shell forgets about the job, so it never passes SIGHUP on
#   The jobs that survive are orphans, so their parent becomes PID 1 (demo 19).
#
#   The script plays both parts. script(1) opens a throwaway pseudo-terminal with an
#   interactive bash inside, which is like opening a new terminal window. Five jobs
#   start in there. Then the pseudo-terminal is killed without warning, which is like
#   clicking the window's close button.
#
# Run
#   ./17_terminal_hangup.sh
#
# What to look for
#   Before: five sleeps. Four of them have bash as their parent and the pseudo-terminal
#   (pts/N) as their TTY. The setsid one already looks different: its own SID, no
#   terminal (?), and PPID 1. That's because setsid had to fork to start a new session,
#   and the in-between process exited right away.
#   After: the plain & job and the stopped job are gone. The nohup, setsid, and disown
#   jobs are still running, now with PPID 1 and no terminal.
#
# Try it by hand
#   Open a new terminal tab and run:
#     sleep 900 &  nohup sleep 901 &  setsid sleep 902 &  sleep 903 & disown
#   Close that tab. Then from another tab:
#     ps -o pid,ppid,sid,tty,stat,cmd -C sleep
#   Only 901, 902, and 903 are left. Then check the nohup one:
#     grep SigIgn /proc/<PID of sleep 901>/status
#   It ends in 1. That bit is SIGHUP (signal 1), marked as ignored.

log=$(mktemp)
# When the script ends, delete the log and kill any sleeps that survived.
trap 'rm -f "$log"; pkill -f "^sleep 91[1-5]$" 2>/dev/null' EXIT

# The commands we'll "type" into the pretend terminal. %% means the most recent job.
# The sleep 0.2 lets job 915 finish exec'ing sleep before we stop it. Stopped any
# earlier, it would still be named bash, and ps -C sleep wouldn't list it.
jobs_to_start='sleep 911 &
nohup sleep 912 >/dev/null 2>&1 &
setsid sleep 913 &
sleep 914 &
disown
sleep 915 &
sleep 0.2
kill -STOP %%
'

# Feed those commands to an interactive bash running inside script(1). script gives it
# a pseudo-terminal, so bash behaves like it's in a real terminal window. The sleep 30
# keeps the input open, so bash doesn't see end-of-input and quit on its own.
{ printf '%s' "$jobs_to_start"; sleep 30; } 2>/dev/null </dev/null |
    script -qfec "bash --norc -i" "$log" >/dev/null 2>&1 &
sleep 2

terminal=$(pgrep -n -f '^script -qfec bash --norc -i')   # the "window"
shell=$(pgrep -P "$terminal" -x bash)                     # the bash inside it

# Print the five sleeps with a label saying how each one was started.
show_jobs() {
    printf "  %-7s %-6s %-7s %-6s %-5s %-17s %s\n" PID PPID SID TTY STAT "started with" COMMAND
    ps -o pid=,ppid=,sid=,tty=,stat=,args= -C sleep | awk '
        BEGIN {
            how["911"] = "plain &";        how["912"] = "nohup ... &"
            how["913"] = "setsid ... &";   how["914"] = "& then disown"
            how["915"] = "& then stopped"
        }
        $6 == "sleep" && ($7 in how) {
            printf "  %-7s %-6s %-7s %-6s %-5s %-17s %s %s\n", $1, $2, $3, $4, $5, how[$7], $6, $7
        }'
}

echo "Pretend terminal window: PID $terminal.  Interactive bash inside it: PID $shell."
echo
echo "--- while the terminal is open"
show_jobs

# "Close the window": kill the pseudo-terminal's owner. bash gets SIGHUP from the kernel.
kill -KILL "$terminal"
sleep 1

echo
echo "--- one second after the terminal disappeared"
show_jobs
echo
if ps -p "$shell" >/dev/null; then
    echo "bash $shell is still running."
else
    echo "bash $shell is gone: it got SIGHUP, passed it to its jobs, and exited."
fi
