CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wformat=2 -Wundef

PROGRAMS = \
	02_deadlock_detection_sim \
	03_bankers_algorithm_sim

.PHONY: all clean

all: $(PROGRAMS)

clean:
	rm -f $(PROGRAMS)
