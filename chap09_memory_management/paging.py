from manim import *


class PagingFaithfulToBook(Scene):
    def construct(self):
        self.camera.background_color = "#ffffff"

        # --- Config & Data ---
        PAGE_SIZE = 4
        mappings = [5, 6, 1, 2]  # Page 0->F5, P1->F6, P2->F1, P3->F2
        logical_chars = "abcdefghijklmnop"

        # --- 1. Logical Memory ---
        logical_mem = VGroup()
        for i, char in enumerate(logical_chars):
            cell = Rectangle(width=0.8, height=0.3, stroke_color=GRAY, stroke_width=1)
            label = Text(str(i), font_size=12, color=GRAY).next_to(cell, LEFT, buff=0.1)
            val = Text(char, font_size=14, color=BLACK).move_to(cell)
            logical_mem.add(VGroup(cell, label, val))
        logical_mem.arrange(DOWN, buff=0)
        log_title = Text("logical memory", font_size=14, color=BLACK).next_to(
            logical_mem, DOWN
        )

        # --- 2. Page Table ---
        pt_rows = VGroup()
        for i, frame in enumerate(mappings):
            p_idx = Text(str(i), font_size=12, color=BLACK)
            box = Rectangle(width=0.5, height=0.4, stroke_color=BLACK, stroke_width=1)
            f_val = Text(str(frame), font_size=14, color=BLACK).move_to(box)
            pt_rows.add(VGroup(p_idx, box, f_val).arrange(RIGHT, buff=0.1))
        pt_rows.arrange(DOWN, buff=0)
        pt_title = Text("page table", font_size=14, color=BLACK).next_to(pt_rows, DOWN)

        # --- 3. Physical Memory ---
        phys_mem = VGroup()
        # Data placement based on the book:
        # Frame 1: Page 2 (i,j,k,l) | Frame 2: Page 3 (m,n,o,p)
        # Frame 5: Page 0 (a,b,c,d) | Frame 6: Page 1 (e,f,g,h)
        frame_data = {1: "ijkl", 2: "mnop", 5: "abcd", 6: "efgh"}

        for f in range(8):  # 0 to 7
            frame_box = VGroup()
            data = frame_data.get(f, "    ")  # Empty if not mapped
            for b in range(4):
                fill = "#d1e8ff" if f in frame_data else WHITE
                cell = Rectangle(
                    width=0.8,
                    height=0.3,
                    stroke_color=GRAY,
                    stroke_width=1,
                    fill_color=fill,
                    fill_opacity=0.5,
                )
                val = Text(data[b], font_size=14, color=BLACK).move_to(cell)
                frame_box.add(VGroup(cell, val))
            frame_box.arrange(DOWN, buff=0)
            addr = (
                Text(str(f * 4), font_size=12, color=GRAY)
                .next_to(frame_box, LEFT, buff=0.1)
                .align_to(frame_box, UP)
            )
            phys_mem.add(VGroup(frame_box, addr))

        phys_mem.arrange(DOWN, buff=0)
        phys_title = Text("physical memory", font_size=14, color=BLACK).next_to(
            phys_mem, DOWN
        )

        # --- Layout and Animation ---
        ui = (
            VGroup(
                VGroup(logical_mem, log_title),
                VGroup(pt_rows, pt_title),
                VGroup(phys_mem, phys_title),
            )
            .arrange(RIGHT, buff=1.2)
            .scale(0.8)
        )

        self.add(ui)

        # Animation: Highlight address 13
        # 13 is in Page 3, Offset 1. PT[3] = 2. Frame 2, Offset 1 = Address 9.
        l_highlight = SurroundingRectangle(logical_mem[13], color=RED)
        pt_highlight = SurroundingRectangle(pt_rows[3], color=BLUE)

        # In phys_mem, index 2 is Frame 2. Its bytes are in sub-group 0.
        # phys_mem[2][0][1] is: [Frame 2][Bytes Group][Byte index 1]
        p_highlight = SurroundingRectangle(phys_mem[2][0][1], color=GREEN)

        self.play(Create(l_highlight))
        self.wait(0.5)
        self.play(Create(pt_highlight))
        self.wait(0.5)
        self.play(Create(p_highlight))
        self.wait(2)
