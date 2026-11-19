# Chapter 8, Day 2. Avoidance comes before detection.
CC = gcc
CFLAGS = -O2 -Wall -Wextra
PROGS = bankers detect

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

run: all
	$(MAKE) 01-avoidance
	@echo "Slides 21-27: introduce current waits and trace the detection example before continuing."
	$(MAKE) 02-detection
	@echo "Slides 29-35: discuss detection timing and recovery. These programs simulate snapshots; they do not recover live mutexes."

01-avoidance: bankers
	@echo "Slides 13-20: Can we grant a request and still guarantee a completion order?"
	./bankers

02-detection: detect
	@echo "Slides 25-28: Given current requests, which threads cannot finish?"
	./detect

clean:
	rm -f $(PROGS)

.PHONY: all run clean 01-avoidance 02-detection
