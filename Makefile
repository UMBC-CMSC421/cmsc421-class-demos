
CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -Wpedantic

all:
	@echo "Build demos by entering a chapter directory and running make"

clean:
	find . -name "*.out" -delete
