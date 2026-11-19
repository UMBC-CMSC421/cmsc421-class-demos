# Makefile for 02_binding: Day 1 demos on address binding and memory cost.
#
#   make          builds prog1, where, and cost
#   make clean    removes them and prog1.o
#
# No -Wpedantic: where.c casts a function pointer to void *, which POSIX allows
# and ISO C does not. cost uses -O2 so the timing loops are tight.

CC     = gcc
CFLAGS = -std=gnu11 -Wall -Wextra -Wshadow -Wconversion -Wformat=2

.PHONY: all clean
all: prog1 where cost

prog1: prog1.c
	$(CC) $(CFLAGS) -c prog1.c
	$(CC) -o prog1 prog1.o

where: where.c
	$(CC) $(CFLAGS) -o where where.c

cost: cost.c
	$(CC) $(CFLAGS) -O2 -o cost cost.c

clean:
	rm -f prog1 prog1.o where cost
