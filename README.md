
# CMSC421 Demos
Operating Systems Demonstration Code and Teaching Materials

This repository contains **C-based demonstrations of core operating system concepts**
used in **CMSC421: Operating Systems**. The demos are organized by textbook chapter
and are designed for classroom demonstrations, labs, and guided exploration.

All examples are written in **C for Linux systems**.

---

## Repository Structure

CMSC421_Demos/
├── README.md
├── Makefile
├── .gitignore
├── shared/
├── chap01_intro/
├── chap02_os_structures/
├── chap03_processes/
├── chap04_threads/
├── chap05_cpu_scheduling/
├── chap06_synchronization/
├── chap07_deadlocks/
├── chap08_memory_management/
├── chap09_virtual_memory/
├── chap10_filesystems/
└── docs/

Each chapter contains demo programs and a README explaining the concept.

---

## Requirements

Linux environment with:

- gcc
- make
- pthread library

Install build tools if needed:

sudo apt install build-essential

---

## Build Example

cd chap03_processes
make

---

## Example Topics

Chapter 3 – Processes
- fork()
- exec()
- zombie processes
- signals

Chapter 4 – Threads
- pthread creation
- shared memory
- race conditions
- mutex locks

Chapter 5 – CPU Scheduling
- scheduling simulations
- round-robin
- priority scheduling

Chapter 6 – Synchronization
- race conditions
- semaphores
- producer/consumer

---

## How to Use

1. Read the chapter in the textbook
2. Navigate to the corresponding chapter folder
3. Build and run the demo
4. Observe behavior using tools like:

ps
top
strace
/proc

---

Educational repository for CMSC421.
