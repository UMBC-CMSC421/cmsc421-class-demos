CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wformat=2 -Wundef
PTHREADFLAGS = -pthread

PROGRAMS = \
	01_dispatcher \
	02_kernel_sched \
	03_nice_cpu_share \
	04_pthread_sched_attr

ARCHIVE_DIR = archive_textbook_examples
ARCHIVE_PROGRAMS = \
	$(ARCHIVE_DIR)/posix-sched \
	$(ARCHIVE_DIR)/posix-rt

.PHONY: all smoke archive clean

all: $(PROGRAMS)

01_dispatcher: 01_dispatcher.c
	$(CC) $(CFLAGS) $< -o $@

02_kernel_sched: 02_kernel_sched.c
	$(CC) $(CFLAGS) $< -o $@

03_nice_cpu_share: 03_nice_cpu_share.c
	$(CC) $(CFLAGS) $< -o $@

04_pthread_sched_attr: 04_pthread_sched_attr.c
	$(CC) $(CFLAGS) $(PTHREADFLAGS) $< -o $@

# Quick check of the demos that need no sudo (under 10 seconds).
smoke: $(PROGRAMS)
	@output=$$(./01_dispatcher fcfs); printf '%s\n' "$$output" | grep -q 'P4   3       5     26'
	@output=$$(./01_dispatcher srtf); printf '%s\n' "$$output" | grep -q 'Average waiting time    = 6.5'
	@output=$$(./03_nice_cpu_share 5 1); printf '%s\n' "$$output" | grep -q 'scheduler weights predict 24.7%'
	@output=$$(./04_pthread_sched_attr); printf '%s\n' "$$output" | grep -q 'policy SCHED_OTHER'
	@echo 'chapter 5 smoke tests passed'

# Textbook examples, kept as written, so they are built with plain warnings only.
archive: $(ARCHIVE_PROGRAMS)

$(ARCHIVE_DIR)/%: $(ARCHIVE_DIR)/%.c
	$(CC) -Wall $(PTHREADFLAGS) $< -o $@

clean:
	rm -f $(PROGRAMS) $(ARCHIVE_PROGRAMS)
