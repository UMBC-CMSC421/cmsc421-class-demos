# Chapter 9: build the existing walkthroughs and the memory models.
.PHONY: all check
all:
	$(MAKE) -C 01_compile_steps all
	$(MAKE) -C 02_binding all
	$(MAKE) -C 03_memory_models all

check: all
	$(MAKE) -C 03_memory_models check
