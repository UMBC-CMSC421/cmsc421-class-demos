CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wformat=2 -Wundef
PTHREADFLAGS = -pthread

PROGRAMS = \
	01_race_condition \
	02_mutex \
	03_semaphore_producer_consumer \
	04_condition_variable \
	05_deadlock_demo \
	06_process_race_and_deadlock

.PHONY: all clean

all: $(PROGRAMS)

06_process_race_and_deadlock: 06_process_race_and_deadlock.c
	$(CC) $(CFLAGS) $< -o $@

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
