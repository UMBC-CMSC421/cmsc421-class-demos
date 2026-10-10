# Chapter 6, synchronization: one counter, successive attempts and solutions.
# make          build everything
# make run      build and run every version in lecture order
# make clean    remove the programs
CC     = gcc
CFLAGS = -O2 -Wall -pthread

PROGS = v1_race interrupts_user v2_turns v3_peterson v4_fence \
        v5_tas v5b_cas v6_atomic v7_atomic_limit

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

run: all
	@echo "== v1: no protection (run 3 times)"
	./v1_race; ./v1_race; ./v1_race
	@echo "== disabling interrupts from a user program"
	-./interrupts_user
	@echo "== v2: strict alternation, equal work"
	./v2_turns 1000000 1000000
	@echo "== v2: strict alternation, unequal work (killed after 5 s if it hangs)"
	-timeout 5 ./v2_turns 3 1 || echo "exit $$?: 124 means it hung and was killed"
	@echo "== v3: Peterson, plain variables (run 3 times; all 3 may still print 2000000)"
	./v3_peterson; ./v3_peterson; ./v3_peterson
	@echo "== v4: Peterson plus a memory barrier"
	./v4_fence; ./v4_fence; ./v4_fence
	@echo "== v5: test_and_set spinlock"
	./v5_tas
	@echo "== v5b: compare_and_swap spinlock"
	./v5b_cas
	@echo "== v6: atomic counter"
	./v6_atomic
	@echo "== v7: limits of atomic variables"
	./v7_atomic_limit

clean:
	rm -f $(PROGS)

.PHONY: all run clean
