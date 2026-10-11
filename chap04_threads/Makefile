CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wformat=2 -Wundef
PTHREADFLAGS = -pthread
OPENMPFLAGS = -fopenmp

PROGRAMS = \
	01_pthread_create_join \
	02_thread_args \
	03_shared_counter_race \
	04_mutex_counter_fix \
	06_word_count_threads \
	07_threads_vs_processes \
	08_shared_array_no_mutex

OPTIONAL_PROGRAMS = 05_openmp_intro

ARCHIVE_DIR = archive_textbook_examples
ARCHIVE_PROGRAMS = \
	$(ARCHIVE_DIR)/fig4-23 \
	$(ARCHIVE_DIR)/pthread1 \
	$(ARCHIVE_DIR)/thrd-posix \
	$(ARCHIVE_DIR)/threads \
	$(ARCHIVE_DIR)/word_threads \
	$(ARCHIVE_DIR)/openmp

.PHONY: all openmp smoke archive clean

all: $(PROGRAMS)

openmp: $(OPTIONAL_PROGRAMS)

07_threads_vs_processes: 07_threads_vs_processes.c
	$(CC) $(CFLAGS) $(PTHREADFLAGS) $< -o $@

01_pthread_create_join: 01_pthread_create_join.c
	$(CC) $(CFLAGS) $(PTHREADFLAGS) $< -o $@

02_thread_args: 02_thread_args.c
	$(CC) $(CFLAGS) $(PTHREADFLAGS) $< -o $@

03_shared_counter_race: 03_shared_counter_race.c
	$(CC) $(CFLAGS) $(PTHREADFLAGS) $< -o $@

04_mutex_counter_fix: 04_mutex_counter_fix.c
	$(CC) $(CFLAGS) $(PTHREADFLAGS) $< -o $@

05_openmp_intro: 05_openmp_intro.c
	$(CC) $(CFLAGS) $(OPENMPFLAGS) $< -o $@

06_word_count_threads: 06_word_count_threads.c
	$(CC) $(CFLAGS) $(PTHREADFLAGS) $< -o $@

08_shared_array_no_mutex: 08_shared_array_no_mutex.c
	$(CC) $(CFLAGS) $(PTHREADFLAGS) $< -o $@

smoke: $(PROGRAMS)
	@output=$$(./01_pthread_create_join); printf '%s\n' "$$output" | grep -q 'thread joined successfully'
	@output=$$(./02_thread_args); printf '%s\n' "$$output" | grep -q 'square=25'
	@output=$$(./03_shared_counter_race); printf '%s\n' "$$output" | grep -q 'Expected value = 400000'
	@output=$$(./04_mutex_counter_fix); printf '%s\n' "$$output" | grep -q 'Actual value   = 400000'
	@output=$$(./06_word_count_threads); printf '%s\n' "$$output" | grep -q '\[result\] total=15 expected=15 CORRECT'
	@output=$$(printf '\n' | ./07_threads_vs_processes); printf '%s\n' "$$output" | grep -q 'main thread after the child exited'
	@output=$$(./08_shared_array_no_mutex); printf '%s\n' "$$output" | grep -q 'first value is still 20'
	@echo 'chapter 4 core smoke tests passed'

# Older examples, kept as written, so they are built with plain warnings only.
archive: $(ARCHIVE_PROGRAMS)

$(ARCHIVE_DIR)/openmp: $(ARCHIVE_DIR)/openmp.c
	$(CC) -Wall $(OPENMPFLAGS) $< -o $@

$(ARCHIVE_DIR)/%: $(ARCHIVE_DIR)/%.c
	$(CC) -Wall $(PTHREADFLAGS) $< -o $@

clean:
	rm -f $(PROGRAMS) $(OPTIONAL_PROGRAMS) $(ARCHIVE_PROGRAMS)
