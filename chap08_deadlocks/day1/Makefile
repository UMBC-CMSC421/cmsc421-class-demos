# Chapter 8, Day 1. Run one numbered target when its slides are reached.
CC = gcc
CFLAGS = -g -O2 -Wall -Wextra -pthread
PROGS = two_mutex livelock transfer errorcheck

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

tsan: two_mutex.c
	$(CC) -g -O1 -Wall -Wextra -fsanitize=thread -pthread two_mutex.c -o two_mutex_tsan

# Each recursive make finishes before the next stage, even with make -j.
run: all
	$(MAKE) 01-deadlock
	$(MAKE) 02-livelock
	$(MAKE) 03-prevention
	$(MAKE) 04-transfer
	@echo "Slide 31: make 05-lock-order-check (optional ThreadSanitizer)"
	@echo "Slide 40: make 06-enrichment (optional self-relock example)"

01-deadlock: two_mutex
	@echo "Slides 7-11: Here is the deadlock. Each thread holds one lock and waits for the other."
	@timeout 4 ./two_mutex textbook; status=$$?; \
	if [ $$status -eq 124 ]; then echo "Stopped after 4 seconds. Identify both holders and both waits."; \
	else echo "Expected a timeout; got status $$status. Inspect this run."; exit 1; fi
	@echo "For live ps/gdb inspection, run ./two_mutex textbook in one terminal and inspect from another."

02-livelock: livelock
	@echo "Slides 12-14: Releasing and retrying can also make no progress."
	./livelock lockstep
	@echo "Now add random backoff. Observe whether both threads finish."
	./livelock backoff
	@echo "Stop for slides 15-26: four conditions, resource graphs, and prevention."

03-prevention: two_mutex
	@echo "Slide 27: Return to the same two locks. Both threads now acquire first_mutex before second_mutex."
	./two_mutex ordered
	@echo "Which necessary condition did the common lock order remove?"

04-transfer: transfer
	@echo "Slides 28-29: Source-then-destination is not a global lock order."
	@timeout 4 ./transfer naive; status=$$?; \
	if [ $$status -eq 124 ]; then echo "Naive transfer did not finish within 4 seconds."; \
	else echo "Expected a timeout; got status $$status. Inspect this run."; exit 1; fi
	@echo "Now order both account locks by stable account id."
	./transfer ordered

05-lock-order-check: tsan
	@echo "Slides 30-31: A completed run can still contain conflicting lock orders."
	@TSAN_OPTIONS=detect_deadlocks=1:exitcode=66 ./two_mutex_tsan serial; status=$$?; \
	if [ $$status -ne 0 ]; then \
	  echo "ThreadSanitizer exited $$status. Read the report: a runtime failure is not a lock-order warning."; \
	  exit $$status; \
	fi

06-enrichment: errorcheck
	@echo "Slide 40, enrichment: one thread tries to lock its own mutex again."
	./errorcheck

clean:
	rm -f $(PROGS) two_mutex_tsan

.PHONY: all tsan run clean 01-deadlock 02-livelock 03-prevention 04-transfer 05-lock-order-check 06-enrichment
