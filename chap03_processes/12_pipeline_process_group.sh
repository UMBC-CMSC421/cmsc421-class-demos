#!/bin/bash
# Chapter 3: Processes
# Demo 12: A pipeline is several processes in one process group
#
# What's going on
#   When you type a | b | c, the shell starts all three programs at the same time.
#   Each one is its own process, and pipes connect them (Fox 4.1).
#   The shell puts all three into ONE process group. That's why Ctrl-C stops a whole
#   pipeline at once: the terminal sends SIGINT to the entire foreground group. From
#   another terminal, kill -- -PGID (with a minus sign in front of the number) does
#   the same thing.
#   A pipeline's exit status ($?) comes from the last command only. bash saves all of
#   them in an array called PIPESTATUS.
#
# Run
#   ./12_pipeline_process_group.sh
#
# What to look for
#   Three sleeps with three different PIDs but the same PGID, which is the PID of the
#   first one. One kill to the group, and zero are left. At the end, $? is 3 but
#   PIPESTATUS shows all three statuses: 1 0 3.
#
# Try it by hand
#   sleep 300 | sleep 301 | sleep 302 &
#   jobs -l                      one job number, three PIDs
#   ps -o pid,pgid,stat,cmd      the three sleeps share one PGID
#   kill %1                      ends the whole job, all three at once
#                                (use the job number jobs showed you)

# Scripts normally run without job control. set -m turns it on, so each background job
# gets its own process group, the same as when you type commands yourself.
set -m

echo "This script is PID $$, process group $(ps -o pgid= -p $$ | tr -d ' ')"
echo
echo "\$ sleep 300 | sleep 301 | sleep 302 &"
# sleep never reads its input. That's fine: all we want is three processes in a pipeline.
# shellcheck disable=SC2216
sleep 300 | sleep 301 | sleep 302 &
last_stage=$!          # $! is the PID of the last process in that pipeline (sleep 302)
sleep 0.5              # give all three a moment to start

pgid=$(ps -o pgid= -p "$last_stage" | tr -d ' ')
echo
echo "--- one job, three processes, one PGID ($pgid):"
ps -o pid,ppid,pgid,sid,stat,cmd -p "$(pgrep -d, -g "$pgid")"    # pgrep -g lists everyone in a group

echo
echo "\$ kill -INT -- -$pgid        # a negative PID means 'every process in that group'"
kill -INT -- "-$pgid"  # the -- stops kill from reading -NNNN as an option
wait 2>/dev/null       # reap them
sleep 0.2
remaining=$(pgrep -g "$pgid" | wc -l)
echo "processes left in group $pgid: $remaining"

echo
echo "--- exit status of a pipeline"
echo "\$ false | true | sh -c 'exit 3'"
false | true | sh -c 'exit 3'      # exits with 1, 0, and 3
echo "\$? = $?  (only the last stage)    PIPESTATUS = ${PIPESTATUS[*]}"
