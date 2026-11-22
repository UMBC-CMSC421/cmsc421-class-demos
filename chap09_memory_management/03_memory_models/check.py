#!/usr/bin/env python3
"""Check the teaching examples, address boundaries, and rejected input on Linux."""
from pathlib import Path
import re
import subprocess
import unittest

ROOT = Path(__file__).resolve().parent
PROGRAMS = ["01_base_limit", "02_allocation", "03_page_translation", "04_tlb",
            "05_page_protection", "06_two_level"]


def run(name, *args, success=True):
    result = subprocess.run([str(ROOT / name), *map(str, args)], input="", text=True,
                            capture_output=True, timeout=10)
    if success and result.returncode != 0:
        raise AssertionError(f"{name} exited {result.returncode}: {result.stderr}")
    if not success and result.returncode == 0:
        raise AssertionError(f"{name} accepted invalid arguments {args}")
    return result.stdout


class MemoryDemos(unittest.TestCase):
    def test_base_limit_boundaries_and_relocation(self):
        output = run("01_base_limit")
        self.assertIn("14000 + 346 = 14346", output)
        self.assertIn("14000 + 999 = 14999", output)
        self.assertIn("22000 + 346 = 22346", output)
        for n in (1000, 1001, 4294967295):
            with self.subTest(address=n):
                rejected = run("01_base_limit", n)
                self.assertIn("TRAP:", rejected)
                self.assertNotIn("Physical address =", rejected)
        self.assertIn("14000 + 0 = 14000", run("01_base_limit", 0))

    def test_paging_every_byte_and_end_boundary(self):
        # Independent specification: four complete logical pages map to these frames.
        frames = [5, 6, 1, 2]
        for address in range(16):
            with self.subTest(address=address):
                output = run("03_page_translation", address)
                expected = frames[address // 4] * 4 + address % 4
                self.assertIn(f"physical_memory[{expected}] = '{chr(97 + address)}'", output)
        for n in (16, 17, 4294967295):
            self.assertIn("INVALID:", run("03_page_translation", n))
        self.assertIn("physical_memory[9] = 'n'", run("03_page_translation", "0xD"))

    def test_paging_hex_process_isolation_and_rounding(self):
        output = run("03_page_translation")
        self.assertIn("physical_memory[29] = 'X'", output)
        self.assertIn("= 0x3A7C", output)
        self.assertIn("Frames = 3; allocated = 3072; unused tail = 1023", output)

    def test_allocation_policies_failure_compaction_and_coalescing(self):
        output = run("02_allocation")
        for policy, hole, remainder in (("FIRST FIT", 500, 288), ("BEST FIT", 300, 88),
                                         ("WORST FIT", 600, 388)):
            part = output.split(policy + "\n", 1)[1].split("Total free", 1)[0]
            self.assertIn(f"selected hole {hole}; remainder {remainder}", part)
        fragment = output.split("External fragmentation:", 1)[1]
        self.assertIn("Total free = 8; largest hole = 5\nRequest 6: NO FIT", fragment)
        self.assertIn("selected hole 8; remainder 2", fragment)
        self.assertTrue(fragment.rstrip().endswith("Total free = 8; largest hole = 8"))
        # Every displayed map covers the same memory, with no overlap or gap.
        for part in re.split(r"Total free = \d+; largest hole = \d+", output)[:-1]:
            blocks = re.findall(r"\[(\d+),(\d+)\)", part)
            self.assertEqual(int(blocks[0][0]), 0)
            for left, right in zip(blocks, blocks[1:]):
                self.assertEqual(left[1], right[0])
            self.assertIn(int(blocks[-1][1]), (1704, 16))

    def test_tlb_fifo_counts_and_process_tags(self):
        output = run("04_tlb")
        self.assertIn("hits = 3; misses = 3; data accesses = 6; table reads = 3", output)
        self.assertIn("Memory reads = 9; at 10 ns per read, modeled EAT = 15.0 ns", output)
        switch = output.split("Switch processes", 1)[1]
        process_b = switch.split("Process A", 1)[0]
        self.assertIn("TLB MISS", process_b)
        self.assertIn("Physical address = 29", process_b)
        self.assertIn("Physical address = 9", switch.split("Process A", 1)[1])

    def test_linux_page_permissions(self):
        output = run("05_page_protection")
        self.assertEqual(re.findall(r"Result: (.+)", output),
                         ["access succeeded", "SIGSEGV", "access succeeded", "access succeeded", "SIGSEGV"])
        self.assertIn("All five permission checks matched.", output)

    def test_two_level_boundaries_and_sparse_tables(self):
        for base, physical in ((0x00403000, 0x1F7000), (0x00801000, 0x220000)):
            for offset in (0, 1, 0xABC, 0xFFF):
                with self.subTest(base=base, offset=offset):
                    self.assertIn(f"= 0x{physical + offset:X}", run("06_two_level", hex(base + offset)))
        self.assertIn("inner entry absent", run("06_two_level", "0x00404000"))
        for address in (0, 0x00C01000, 0xFFFFFFFF):
            self.assertIn("outer entry absent", run("06_two_level", hex(address)))
        self.assertIn("2 leaf tables * 4096 = 12288", run("06_two_level"))
        self.assertIn("1048576 entries * 4 = 4194304", run("06_two_level"))

    def test_bad_arguments_fail(self):
        for name in PROGRAMS:
            for value in ("-1", "4294967296", "12junk", "0x", "", " 1", "--unknown"):
                with self.subTest(program=name, value=value):
                    run(name, value, success=False)
            run(name, "1", "2", success=False)
            run(name, "--step", "--step", success=False)

    def test_step_mode_preserves_results(self):
        for name in PROGRAMS:
            with self.subTest(program=name):
                self.assertEqual(run(name), run(name, "--step").replace("Press Enter to continue...\n", ""))


if __name__ == "__main__":
    unittest.main(verbosity=2)
