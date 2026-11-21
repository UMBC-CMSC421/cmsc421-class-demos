# CMSC 421, Chapter 10: Virtual Memory

Use the course Linux VM to compare virtual address space, resident memory, and
page faults. Read each program's opening comment before running it.

## Build

```sh
make
```

## Demos

| File | Run | What to observe |
|---|---|---|
| `01_memory_hog.c` | `./01_memory_hog 200` | Allocates and writes 10 MiB at a time, up to the requested limit. The printed MB values are in units of 1,048,576 bytes. |
| `02_memory_limits_oom.sh` | `./02_memory_limits_oom.sh` | An address-space limit can make `malloc()` fail. A cgroup memory limit can cause the OOM killer to terminate the process. |
| `03_lazy_alloc.c` | `./03_lazy_alloc` | Maps 4 GiB, writes one page, then writes one byte per page across 256 MiB. Compare virtual size, resident size, and fault counts after each step. |
| `04_cow_fork.c` | `./04_cow_fork` | The child reads and then writes a 64 MiB buffer inherited through `fork()`. Compare the child's minor faults and check that the parent's first byte stays unchanged. |

Demo 02 needs a working user systemd session with memory-controller support.
It uses `sudo dmesg` to inspect kernel messages. Check the actual exit status:
137 is consistent with SIGKILL, but the kernel message establishes whether the
OOM killer caused it.

Demo 03 requests base pages with `madvise(MADV_NOHUGEPAGE)` so huge pages do not
hide the per-page pattern. On the course VM, base pages are 4 KiB. Compare changes
between reports; fault totals also include other work done by the process.

For demo 04, reading the inherited data does not require a private copy of each
page. Writing shared data triggers copy-on-write. Exact fault counts can vary.

`virtual.py` contains older Manim scenes and is separate from the C demos.
