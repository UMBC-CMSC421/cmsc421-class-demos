CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wformat=2 -Wundef

PROGRAMS = \
	01_fcfs_sim \
	02_sjf_sim \
	03_rr_sim \
	04_priority_sim \
	05_posix_sched_demo

.PHONY: all clean

all: $(PROGRAMS)

clean:
	rm -f $(PROGRAMS)
