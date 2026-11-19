#!/bin/bash
# run_steps.sh - build main.c and greet.c one step at a time and show what
# each step produced: preprocess, compile, assemble, link, load, run.
#
# Usage:  ./run_steps.sh        pause before each step (press Enter to go on)
#         ./run_steps.sh -n     run straight through, no pauses
#
# Every file a step makes goes in steps/. "make clean" removes it.

cd "$(dirname "$0")" || exit 1
set -e

PAUSE=1
if [ "$1" = "-n" ]; then
    PAUSE=0
fi

# Same flags as the Makefile: -O0 keeps the assembly close to the C,
# and -fno-asynchronous-unwind-tables leaves out the .cfi lines.
CFLAGS="-Wall -Wextra -O0 -fno-asynchronous-unwind-tables"

# step TITLE: print a banner, then wait for Enter when pausing is on.
step() {
    echo
    echo "=================================================================="
    echo "$1"
    echo "=================================================================="
    if [ "$PAUSE" = 1 ]; then
        read -r -p "(press Enter) " _
    fi
}

# show CMD...: print a command, then run it, so the class sees both.
show() {
    echo
    echo "\$ $*"
    eval "$@"
}

rm -rf steps
mkdir steps

# ---------------------------------------------------------------- step 0
step "Step 0: the source. Two .c files and one header."
show "wc -l main.c greet.c greet.h"

# ---------------------------------------------------------------- step 1
step "Step 1: preprocess (gcc -E). #include pastes files in; macros are replaced."
show "gcc -E main.c -o steps/main.i"
show "gcc -E greet.c -o steps/greet.i"
# greet.i is long because <stdio.h> and every header it includes were pasted in.
show "wc -l main.c steps/main.i greet.c steps/greet.i"
# The end of main.i: the header's declaration of greet, and TIMES is now 3.
# Lines starting with # are the preprocessor's notes on where each line came from;
# this view hides them and the blank lines.
show "grep -v '^#' steps/main.i | grep -v '^\$' | tail -n 10"
# printf's declaration came from <stdio.h>; its code is still elsewhere.
show "grep -n 'extern int printf' steps/greet.i"

# ---------------------------------------------------------------- step 2
step "Step 2: compile (gcc -S). C becomes assembly for this CPU (x86-64)."
show "gcc $CFLAGS -S steps/main.i -o steps/main.s"
show "gcc $CFLAGS -S steps/greet.i -o steps/greet.s"
# main as assembly: the loop, the compare with 3, and "call greet".
show "sed -n '/^main:/,/ret/p' steps/main.s"

# ---------------------------------------------------------------- step 3
step "Step 3: assemble (gcc -c). Assembly text becomes a binary object file."
show "gcc -c steps/main.s -o steps/main.o"
show "gcc -c steps/greet.s -o steps/greet.o"
show "file steps/main.o"
# The symbol table. T = code defined here, B = zeroed global, U = used but undefined.
show "nm steps/main.o"
show "nm steps/greet.o"
# The call to greet has no address yet: 00 00 00 00 plus a relocation note
# telling the linker which name to fill in.
show "objdump -d -r steps/main.o | grep -A1 'call'"

# ---------------------------------------------------------------- step 4
step "Step 4: link (gcc main.o greet.o). Join the object files; connect libc."
show "gcc steps/main.o steps/greet.o -o steps/hello"
show "file steps/hello"
# greet is now defined (T). printf is still U: the C library is linked dynamically.
show "nm steps/hello | grep -E ' (main|greet|counter|printf)'"
# The same call now has an offset: the linker filled in where greet is.
show "objdump -d steps/hello --disassemble=main | grep 'call'"

# ---------------------------------------------------------------- step 5
step "Step 5: load. What execve() reads to build the process."
# INTERP names the dynamic loader; each LOAD line becomes a region of memory.
show "readelf -lW steps/hello | grep -E 'INTERP|LOAD|interpreter'"
# The shared libraries the loader will map in when the program starts.
show "ldd steps/hello"

# ---------------------------------------------------------------- step 6
step "Step 6: run. The loader maps libc and connects printf on first use."
show "./steps/hello"
# LD_DEBUG asks the dynamic loader to report each name it connects.
show "LD_DEBUG=bindings ./steps/hello 2>&1 | grep 'symbol .printf'"

# ---------------------------------------------------------------- step 7
step "Step 7: what plain 'gcc main.c greet.c' does: the same steps, in one command."
# -v prints each program gcc runs: cc1 (preprocess and compile), as (assemble),
# collect2 (which runs the linker, ld). Only the program names are shown here.
show "gcc -v main.c greet.c -o steps/hello_all 2>&1 | grep -E '^ (/[^ ]*/)?(cc1|as|collect2) ' | awk '{print \$1}'"
show "./steps/hello_all"

echo
echo "Done. Every step's output is in steps/. Remove it with: make clean"
