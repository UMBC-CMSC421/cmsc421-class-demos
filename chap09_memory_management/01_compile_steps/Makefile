# Makefile for 01_compile_steps: build the program one step at a time.
#
#   make preprocess   main.c, greet.c  ->  steps/main.i, steps/greet.i
#   make compile      .i               ->  steps/main.s, steps/greet.s  (assembly)
#   make assemble     .s               ->  steps/main.o, steps/greet.o  (object files)
#   make link         .o + libc        ->  steps/hello                  (executable)
#   make run          load and run steps/hello
#   make clean        remove steps/
#
# Each target runs the steps before it, so "make link" does all four.
# -O0 keeps the assembly close to the C source. -fno-asynchronous-unwind-tables
# drops the .cfi lines that would otherwise fill the assembly listing.

CC     = gcc
CFLAGS = -Wall -Wextra -O0 -fno-asynchronous-unwind-tables
OUT    = steps

.PHONY: all preprocess compile assemble link run clean
all: link

preprocess: $(OUT)/main.i $(OUT)/greet.i
compile:    $(OUT)/main.s $(OUT)/greet.s
assemble:   $(OUT)/main.o $(OUT)/greet.o
link:       $(OUT)/hello

$(OUT):
	mkdir -p $(OUT)

# Step 1: preprocess only (-E). Headers are pasted in, macros replaced.
$(OUT)/%.i: %.c greet.h | $(OUT)
	$(CC) -E $< -o $@

# Step 2: compile only (-S). C becomes assembly text.
$(OUT)/%.s: $(OUT)/%.i
	$(CC) $(CFLAGS) -S $< -o $@

# Step 3: assemble only (-c). Assembly becomes a binary object file.
$(OUT)/%.o: $(OUT)/%.s
	$(CC) -c $< -o $@

# Step 4: link. Join the object files and connect them to the C library.
$(OUT)/hello: $(OUT)/main.o $(OUT)/greet.o
	$(CC) $^ -o $@

run: $(OUT)/hello
	./$(OUT)/hello

clean:
	rm -rf $(OUT)
