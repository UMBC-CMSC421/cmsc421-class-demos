# Chapter 6, synchronization, Day 2: the tools, in lecture order.
# make          build everything
# make run      build and run every demo in lecture order (under a minute)
# make clean    remove the programs
CC     = gcc
CFLAGS = -O2 -Wall -pthread

PROGS = v8_mutex nested spin_vs_sleep bank rwlock sem_permits sem_order sem_barge \
        condvar deadlock dining cas_vs_mutex

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

run: all
	@echo "== v8: Day 1's counter with a mutex"
	./v8_mutex
	@echo "== two locks nested in one order: a, then b; release b, then a"
	./nested
	@echo "== spinlock vs. mutex, 2 threads, then 4 threads per CPU (you have $$(nproc) CPUs)"
	./spin_vs_sleep spin 2; ./spin_vs_sleep mutex 2
	./spin_vs_sleep spin $$((4 * $$(nproc))); ./spin_vs_sleep mutex $$((4 * $$(nproc)))
	@echo "== Activity 6.5.2: where does the lock go in withdraw()?"
	./bank none; ./bank inside; ./bank whole
	@echo "== reader-writer lock: mutex, default rwlock (prefers readers), writer preference"
	./rwlock mutex; ./rwlock rw; ./rwlock writer
	@echo "== a semaphore with 2 permits, 5 threads"
	./sem_permits 2
	@echo "== ordering with a semaphore that starts at 0"
	./sem_order nosem; ./sem_order sem
	@echo "== who gets the permit sem_post frees? glibc's semaphore, then the textbook's FIFO semaphore"
	./sem_barge posix; ./sem_barge textbook
	@echo "== condition variable: while (correct), then if (bug)"
	./condvar while
	./condvar if
	@echo "== deadlock: opposite lock order (killed after 4 s), then one order"
	-timeout 4 ./deadlock opposite || echo "exit $$?: 124 means it hung and was killed"
	./deadlock ordered
	@echo "== dining philosophers: everyone takes the left first (killed after 4 s), then asymmetric"
	-timeout 4 ./dining left || echo "exit $$?: 124 means it hung and was killed"
	./dining asymmetric
	@echo "== compare-and-swap vs. mutex as threads grow"
	for t in 1 2 4 8; do ./cas_vs_mutex cas $$t; ./cas_vs_mutex mutex $$t; done

clean:
	rm -f $(PROGS)

.PHONY: all run clean
