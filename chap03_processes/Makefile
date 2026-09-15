# Chapter 3 demos
#
#   make                  build everything (demos 11-21 too, if you're on Linux)
#   make 04_fork_basics   build just one demo
#   make smoke            quick check that demos 01-10 run
#   make smoke-linux      quick check that demos 12-21 run (Linux only)
#   make clean            delete the built programs and any core file
#
# The long list of -W flags turns on extra compiler warnings, so small mistakes
# show up when you build instead of when you run.

CC ?= cc
CFLAGS =-std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wformat=2 -Wundef
CPPFLAGS = -I../shared -D_POSIX_C_SOURCE=200809L

PROGRAMS = \
	01_proc_identity \
	02_process_attributes \
	03_memory_layout \
	04_fork_basics \
	05_fork_cow \
	06_fork_exec \
	07_zombie_demo \
	08_signals \
	09_context_switch_pressure \
	10_process_state_lifecycle

# Demos 11-21 (processes, following Fox, Linux with Operating System Concepts) use
# Linux-only interfaces, so they are built only on Linux; 01-10 still build on macOS.
LINUX_PROGRAMS = \
	11_foreground_background \
	14_state_codes \
	15_setuid_identity \
	16_signal_dispositions \
	18_core_dump \
	19_orphan_subreaper \
	20_context_switches

LINUX_SCRIPTS = \
	12_pipeline_process_group.sh \
	13_monitoring_tour.sh \
	17_terminal_hangup.sh \
	21_proc_tour.sh

ifeq ($(shell uname -s),Linux)
PROGRAMS += $(LINUX_PROGRAMS)
endif

# The core-dump demo keeps debug symbols for gdb.
18_core_dump: CFLAGS += -g

.PHONY: all smoke smoke-linux clean

all: $(PROGRAMS)
	@chmod +x $(LINUX_SCRIPTS)

smoke: all
	./01_proc_identity
	./02_process_attributes
	./03_memory_layout
	./04_fork_basics
	./05_fork_cow
	./06_fork_exec
	./09_context_switch_pressure 10
	./10_process_state_lifecycle

# Linux only: a non-interactive pass over demos 12-21 (about 20 seconds).
# Not included: 11 (needs Ctrl-Z in a real terminal), the sudo steps of 15,
# and 18 (crashes on purpose).
smoke-linux: all
	./12_pipeline_process_group.sh
	./13_monitoring_tour.sh
	echo | ./14_state_codes
	./15_setuid_identity
	./16_signal_dispositions
	./17_terminal_hangup.sh
	./19_orphan_subreaper
	./20_context_switches
	./21_proc_tour.sh

clean:
	rm -f $(PROGRAMS) $(LINUX_PROGRAMS) core
