CC = gcc
CFLAGS = -std=c11 -g -O2 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wformat=2 -Wundef -Werror
PROGRAMS = 01_base_limit 02_allocation 03_page_translation 04_tlb 05_page_protection 06_two_level

.PHONY: all check run
all: $(PROGRAMS)

%: %.c walkthrough.h
	$(CC) $(CFLAGS) $< -o $@

check: all
	python3 check.py

# Each program runs without pauses. Use --step on one program during lecture.
run: all
	@set -e; for program in $(PROGRAMS); do ./$$program; done
