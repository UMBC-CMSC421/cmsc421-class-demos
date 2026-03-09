CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wformat=2 -Wundef
CPPFLAGS = -I../shared

PROGRAMS = \
	01_proc_identity \
	02_memory_layout \
	03_fork_basics \
	04_fork_cow \
	05_fork_exec \
	06_zombie_demo \
	07_signals \
	08_context_switch_pressure

.PHONY: all clean

all: $(PROGRAMS)

clean:
	rm -f $(PROGRAMS)
