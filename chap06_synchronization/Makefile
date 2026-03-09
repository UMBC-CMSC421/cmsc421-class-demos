CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wformat=2 -Wundef
PTHREADFLAGS = -pthread

PROGRAMS = \
	01_race_condition \
	02_mutex \
	03_semaphore_producer_consumer \
	04_condition_variable \
	05_deadlock_demo

.PHONY: all clean

all: $(PROGRAMS)

01_race_condition: 01_race_condition.c
	$(CC) $(CFLAGS) $(PTHREADFLAGS) $< -o $@

02_mutex: 02_mutex.c
	$(CC) $(CFLAGS) $(PTHREADFLAGS) $< -o $@

03_semaphore_producer_consumer: 03_semaphore_producer_consumer.c
	$(CC) $(CFLAGS) $(PTHREADFLAGS) $< -o $@

04_condition_variable: 04_condition_variable.c
	$(CC) $(CFLAGS) $(PTHREADFLAGS) $< -o $@

05_deadlock_demo: 05_deadlock_demo.c
	$(CC) $(CFLAGS) $(PTHREADFLAGS) $< -o $@

clean:
	rm -f $(PROGRAMS)
