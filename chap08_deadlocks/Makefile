CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wformat=2 -Wundef

PROGRAMS = \
	02_deadlock_detection_sim \
	03_bankers_algorithm_sim

.PHONY: all legacy day1 day2 run-day1 run-day2 clean

# The current lecture demos live in day1/ and day2/.
all: day1 day2

legacy: $(PROGRAMS)

day1:
	$(MAKE) -C day1 all

day2:
	$(MAKE) -C day2 all

run-day1:
	$(MAKE) -C day1 run

run-day2:
	$(MAKE) -C day2 run

clean:
	rm -f $(PROGRAMS)
