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
#   The jobs that survive are orphans, so they are adopted by PID 1 or by the nearest
#   subreaper, such as systemd --user on a desktop (demo 19).
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
#   terminal (?), and PPID 1 (or a subreaper). That's because setsid had to fork to
#   start a new session, and the in-between process exited right away.
#   After: the plain & job and the stopped job are gone. (Rarely the plain & job
#   survives one run: the shell is racing its own terminal shutdown. Run it again.)
#   The nohup, setsid, and disown jobs are still running, now with no terminal and a
#   new parent: PID 1, or a subreaper such as systemd --user if you run this from a
#   desktop terminal.
#
# Try it by hand
#   Open a new terminal tab and run:
#     sleep 900 &  nohup sleep 901 &  setsid sleep 902 &  sleep 903 & disown
#   Close that tab. Then from another tab:
#     ps -o pid,ppid,sid,tty,stat,cmd -C sleep
#   Only 901, 902, and 903 are left. Then check the nohup one:
#     grep SigIgn /proc/<PID of sleep 901>/status
#   It ends in 1. That bit is SIGHUP (signal 1), marked as ignored.

# mktemp makes an empty temp file and prints its name. script(1) records into it.
log=$(mktemp)
# trap ... EXIT runs those commands when this script ends, however it ends.
# Here that means: delete the log and kill any sleeps that survived.
trap 'rm -f "$log"; pkill -f "^sleep 91[1-5]$" 2>/dev/null' EXIT

# The commands we'll "type" into the pretend terminal. %% means the most recent job.
# huponexit (with a login shell) makes bash pass SIGHUP to its jobs however it exits.
# Without it, bash sometimes sees the dead terminal as end-of-input first and exits
# without resending SIGHUP, so the result would vary from run to run.
# The sleep 0.2 lets job 915 finish exec'ing sleep before we stop it. Stopped any
# earlier, it would still be named bash, and ps -C sleep wouldn't list it.
jobs_to_start='shopt -s huponexit
sleep 911 &
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
# The & at the end runs this whole pipeline in the background, so this script keeps going.
{ printf '%s' "$jobs_to_start"; sleep 30; } 2>/dev/null </dev/null |
    script -qfec "bash --noprofile --norc --login -i" "$log" >/dev/null 2>&1 &
sleep 2

# pgrep -f matches against the full command line, and -n picks the newest match.
terminal=$(pgrep -n -f '^script -qfec bash --noprofile')   # the "window"
shell=$(pgrep -n -f '^bash --noprofile --norc --login -i$')                    # the bash inside it
# (Found by command line, not by parent: script(1) may start bash through $SHELL -c,
#  so bash is not always script's direct child.)

# Print the five sleeps with a label saying how each one was started.
# ps -C sleep lists every process named sleep. The = after each column name drops the
# header. awk keeps only our five (911 to 915) and adds the label from its how table.
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
# Give bash time to get its SIGHUP, pass it on, and exit (up to 5 seconds).
# ps -p succeeds while that PID still exists, so the loop stops early once bash is gone.
for _ in 1 2 3 4 5 6 7 8 9 10; do
    [ -n "$shell" ] && ps -p "$shell" >/dev/null || break
    sleep 0.5
done
sleep 0.5

echo
echo "--- after the terminal disappeared"
show_jobs
echo
if [ -z "$shell" ]; then
    echo "Could not find the interactive bash; is script(1) installed (bsdutils)?"
elif ps -p "$shell" >/dev/null; then
    echo "bash $shell is still running."
else
    echo "bash $shell is gone: it got SIGHUP, passed it to its jobs, and exited."
fi
