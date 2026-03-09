CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wformat=2 -Wundef
PTHREADFLAGS = -pthread

PROGRAMS = \
	demo1 \
	demo2 \
	demo3

.PHONY: all clean

all: $(PROGRAMS)

demo1: demo1_race_counter.c
	$(CC) $(CFLAGS) $(PTHREADFLAGS) $< -o $@

demo2: demo2_mutex_vs_semaphore.c
	$(CC) $(CFLAGS) $(PTHREADFLAGS) $< -o $@

demo3: demo3_condvar_while.c
	$(CC) $(CFLAGS) $(PTHREADFLAGS) $< -o $@

clean:
	rm -f $(PROGRAMS)
