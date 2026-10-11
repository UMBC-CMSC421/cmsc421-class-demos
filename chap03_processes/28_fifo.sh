#!/bin/bash
# Chapter 3: Processes
# Demo 28: Named pipes (FIFOs)
#
# What's going on
#   An ordinary pipe has no name, so only processes that inherit its file descriptors
#   (a common ancestor called pipe() and forked them) can use it. A named pipe, or
#   FIFO, has a path in the file system, so any process with permission can open it.
#   It still holds no data on disk: it's a kernel buffer with a name.
#   Opening a FIFO blocks until the other end is opened too, so a reader and a writer
#   "meet" at open().
#
# Run
#   ./28_fifo.sh
#
# What to look for
#   ls -l shows a "p" as the first character: the file type is pipe, size 0.
#   The reader started first and waited inside open() until the writer showed up:
#   its WCHAN (the kernel function it sleeps in) is wait_for_partner.
#   After rm, the name is gone, like any file.
#
# Try it by hand, in two terminals
#   Terminal 1:  mkfifo /tmp/myfifo; cat /tmp/myfifo        (waits)
#   Terminal 2:  echo hello > /tmp/myfifo                    (terminal 1 prints hello)
#   Then:        rm /tmp/myfifo
#   For two-way traffic, use two FIFOs: one per direction.

# Make a private scratch folder. $$ is this script's PID, so the name is unique.
# The trap deletes the folder when the script exits, even if something fails.
dir=/tmp/cmsc421_fifo_$$
mkdir -p "$dir"
fifo="$dir/myfifo"
trap 'rm -rf "$dir"' EXIT

# Create the FIFO. It's a file system entry, so ls can list it like any file.
echo "\$ mkfifo $fifo"
mkfifo "$fifo"
echo "\$ ls -l $fifo"
ls -l "$fifo"
echo

# Start the reader in the background (the & at the end). cat opens the FIFO and
# gets stuck in open(), because nobody has opened the write end yet.
# sed just puts "reader got:" in front of each line cat prints.
# $! is the PID of the background job we just started.
echo "Starting a reader in the background. It blocks in open() until a writer arrives."
cat "$fifo" | sed 's/^/  reader got: /' &
reader=$!
sleep 1
# Peek at the waiting cat. pgrep -n -x cat finds the newest process named exactly cat.
# STAT shows its state (S means sleeping) and WCHAN shows where in the kernel it waits.
echo "Reader's state while it waits:"
ps -o pid,stat,wchan:20,cmd -p "$(pgrep -n -x cat)"
echo

# Now be the writer. This open() completes the meeting, the line goes through the
# kernel buffer to cat, and when echo closes its end, cat sees EOF and exits.
echo "\$ echo 'hello through a FIFO' > $fifo"
echo 'hello through a FIFO' > "$fifo"
wait "$reader"
echo
# Remove the FIFO and show that the folder is now empty.
echo "\$ rm $fifo"
rm "$fifo"
ls -l "$dir"
