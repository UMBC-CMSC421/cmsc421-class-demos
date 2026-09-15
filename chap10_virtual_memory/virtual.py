from manim import *

# Color conventions
CPU_COLOR = BLUE
RAM_COLOR = GREEN
DISK_COLOR = RED
OS_COLOR = ORANGE
HIGHLIGHT = YELLOW


class VirtualMemoryBackground(Scene):
    def construct(self):
        title = Text("Virtual Memory: Background").to_edge(UP)
        self.play(Write(title))

        # Program blocks
        program = VGroup(
            Rectangle(width=6, height=1, color=WHITE).shift(UP * 1),
            Text("Program (100 MB)").scale(0.5).move_to(UP * 1),
        )

        # RAM
        ram = Rectangle(width=3, height=2, color=RAM_COLOR).shift(LEFT * 4)
        ram_label = Text("RAM (16 MB)").scale(0.5).next_to(ram, DOWN)

        # Disk
        disk = Rectangle(width=3, height=2, color=DISK_COLOR).shift(RIGHT * 4)
        disk_label = Text("Disk").scale(0.5).next_to(disk, DOWN)

        self.play(
            Create(program),
            Create(ram),
            Write(ram_label),
            Create(disk),
            Write(disk_label),
        )

        # Active segment
        active = Rectangle(width=2, height=0.5, color=HIGHLIGHT).move_to(program[0])
        self.play(Create(active))

        # Move active to RAM
        self.play(active.animate.move_to(ram.get_center()))

        # Show multiple programs
        p2 = active.copy().shift(DOWN)
        self.play(FadeIn(p2))

        # CPU
        cpu = Circle(radius=0.5, color=CPU_COLOR).shift(DOWN * 2)
        cpu_label = Text("CPU").scale(0.4).next_to(cpu, DOWN)
        self.play(Create(cpu), Write(cpu_label))

        self.wait()


class VirtualMemoryFailure(Scene):
    def construct(self):
        title = Text("Failure: Page Fault").to_edge(UP)
        self.play(Write(title))

        ram = Rectangle(width=3, height=2, color=RAM_COLOR).shift(LEFT * 3)
        disk = Rectangle(width=3, height=2, color=DISK_COLOR).shift(RIGHT * 3)

        self.play(Create(ram), Create(disk))

        missing = Text("Page not in RAM", color=YELLOW).move_to(UP)
        self.play(Write(missing))

        os = Rectangle(width=2, height=1, color=OS_COLOR).shift(DOWN * 2)
        os_label = Text("OS").scale(0.5).move_to(os.get_center())

        self.play(Create(os), Write(os_label))

        # Load from disk to RAM
        page = Rectangle(width=1, height=0.5, color=YELLOW).move_to(disk)
        self.play(page.animate.move_to(ram))

        self.wait()
