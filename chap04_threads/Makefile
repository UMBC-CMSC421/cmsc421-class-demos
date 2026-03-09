CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wformat=2 -Wundef
PTHREADFLAGS = -pthread
OPENMPFLAGS = -fopenmp

PROGRAMS = \
	01_pthread_create_join \
	02_thread_args \
	03_shared_counter_race \
	04_mutex_counter_fix \
	06_word_count_threads

OPTIONAL_PROGRAMS = 05_openmp_intro

.PHONY: all clean

all: $(PROGRAMS) $(OPTIONAL_PROGRAMS)

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

clean:
	rm -f $(PROGRAMS) $(OPTIONAL_PROGRAMS)
