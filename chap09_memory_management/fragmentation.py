from manim import *


class FragmentationTest(Scene):
    def construct(self):
        self.camera.background_color = "#fdfdfd"

        # --- Titles ---
        title = Text(
            "Memory Fragmentation Analysis", font_size=32, weight=BOLD, color="#0b6ea8"
        )
        title.to_edge(UP, buff=0.3)
        self.add(title)

        # ==========================================
        # PART 1: EXTERNAL FRAGMENTATION
        # ==========================================
        ext_label = Text(
            "1. External Fragmentation (Dynamic)", font_size=22, color="#d02b2b"
        ).next_to(title, DOWN, buff=0.2)
        self.play(Write(ext_label))

        # Create a RAM block
        ram_ext = Rectangle(
            width=2, height=5, stroke_color=BLACK, stroke_width=2
        ).shift(LEFT * 3)
        ram_text = Text("Physical RAM", font_size=16, color=BLACK).next_to(
            ram_ext, UP, buff=0.1
        )
        self.play(Create(ram_ext), FadeIn(ram_text))

        # Load Process A (8 units) and Process B (4 units) with a "hole" in between
        proc_a = (
            Rectangle(
                width=2, height=1.5, fill_color=RED, fill_opacity=0.6, stroke_width=1
            )
            .align_to(ram_ext, UP)
            .shift(LEFT * 3)
        )
        proc_b = (
            Rectangle(
                width=2, height=1.0, fill_color=GREEN, fill_opacity=0.6, stroke_width=1
            )
            .move_to(ram_ext.get_center() + DOWN * 1)
            .shift(LEFT * 3)
        )

        a_lbl = Text("P-A (8)", font_size=14, color=WHITE).move_to(proc_a)
        b_lbl = Text("P-B (4)", font_size=14, color=WHITE).move_to(proc_b)

        self.play(FadeIn(proc_a, shift=DOWN), Write(a_lbl))
        self.play(FadeIn(proc_b, shift=DOWN), Write(b_lbl))

        # Show the "Holes" (Total 8 units, but split)
        hole_marker = Text(
            "Total Free: 8 units\n(Scattered)", font_size=16, color=RED
        ).shift(RIGHT * 0.5)
        arrow1 = Arrow(
            hole_marker.get_left(), ram_ext.get_top() + DOWN * 1.8, color=RED, buff=0.1
        )
        arrow2 = Arrow(
            hole_marker.get_left(), ram_ext.get_bottom() + UP * 0.3, color=RED, buff=0.1
        )

        self.play(Write(hole_marker), Create(arrow1), Create(arrow2))

        # Attempt to fit Process C (10 units)
        proc_c = VGroup(
            Rectangle(
                width=2.5,
                height=2.5,
                fill_color=BLUE,
                fill_opacity=0.3,
                stroke_dash_array=[5, 5],
            ),
            Text("Process C (10)\nCANNOT FIT", font_size=18, color=BLUE, weight=BOLD),
        ).shift(RIGHT * 4)

        self.play(FadeIn(proc_c))
        self.play(Indicate(proc_c, color=RED), Indicate(hole_marker, scale_factor=1.2))
        self.wait(1.5)

        # Clear for next part
        self.play(
            FadeOut(
                VGroup(
                    ram_ext,
                    ram_text,
                    proc_a,
                    proc_b,
                    a_lbl,
                    b_lbl,
                    hole_marker,
                    arrow1,
                    arrow2,
                    proc_c,
                    ext_label,
                )
            )
        )

        # ==========================================
        # PART 2: INTERNAL FRAGMENTATION
        # ==========================================
        int_label = Text(
            "2. Internal Fragmentation (Paging)", font_size=22, color="#2b80d0"
        ).next_to(title, DOWN, buff=0.2)
        self.play(Write(int_label))

        # Fixed Pages (4 units each)
        pages = (
            VGroup(
                *[
                    VGroup(
                        Rectangle(
                            width=3, height=0.8, stroke_color=GRAY_B, stroke_width=1
                        ),
                        Text(f"Frame {i}", font_size=10, color=GRAY).shift(LEFT * 2.2),
                    )
                    for i in range(3)
                ]
            )
            .arrange(DOWN, buff=0)
            .shift(LEFT * 2)
        )

        self.play(Create(pages))

        # Process D (9 units)
        # Fills Frame 0 (4), Frame 1 (4), and 1 unit of Frame 2
        fill_full = VGroup(
            Rectangle(width=3, height=0.8, fill_color=BLUE_E, fill_opacity=0.5).move_to(
                pages[0][0]
            ),
            Rectangle(width=3, height=0.8, fill_color=BLUE_E, fill_opacity=0.5).move_to(
                pages[1][0]
            ),
        )
        fill_partial = Rectangle(
            width=0.75, height=0.8, fill_color=BLUE_E, fill_opacity=0.5
        ).align_to(pages[2][0], LEFT)

        d_title = Text("Process D (9 units)", font_size=16, color=BLACK).next_to(
            pages, UP
        )

        self.play(Write(d_title))
        self.play(FadeIn(fill_full, shift=RIGHT), FadeIn(fill_partial, shift=RIGHT))

        # Highlight internal waste
        waste = Rectangle(
            width=2.25, height=0.8, fill_color=RED, fill_opacity=0.4, stroke_width=0
        ).align_to(pages[2][0], RIGHT)
        waste_text = Text(
            "Internal Waste\n(3 units)", font_size=16, color=RED, weight=BOLD
        ).next_to(waste, RIGHT, buff=0.5)
        w_arrow = Arrow(waste_text.get_left(), waste.get_center(), color=RED)

        self.play(Create(waste), Write(waste_text), Create(w_arrow))
        self.play(Indicate(waste))

        self.wait(3)
