CC=gcc
CFLAGS=-Wall -Wextra -std=c11

PROGRAMS=random_access_demo mmap_demo allocation_sim fat_trace bitmap_alloc tinyfs

all: $(PROGRAMS)

random_access_demo: random_access_demo.c
	$(CC) $(CFLAGS) -o $@ $<

mmap_demo: mmap_demo.c
	$(CC) $(CFLAGS) -o $@ $<

allocation_sim: allocation_sim.c
	$(CC) $(CFLAGS) -o $@ $<

fat_trace: fat_trace.c
	$(CC) $(CFLAGS) -o $@ $<

bitmap_alloc: bitmap_alloc.c
	$(CC) $(CFLAGS) -o $@ $<

tinyfs: tinyfs.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGRAMS) numbers.txt mapped.txt
