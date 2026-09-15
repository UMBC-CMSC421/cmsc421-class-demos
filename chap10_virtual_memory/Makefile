CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wformat=2 -Wundef

PROGRAMS = 01_memory_hog

.PHONY: all clean

all: $(PROGRAMS)
	@chmod +x 02_memory_limits_oom.sh

clean:
	rm -f $(PROGRAMS)
