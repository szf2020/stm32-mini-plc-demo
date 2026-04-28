"""
STM32 Mini PLC - Ladder Editor
A graphical tool to design ladder programs and download them to the STM32 PLC.
"""

import customtkinter as ctk
from tkinter import Canvas

# ---------- Configuration ----------
ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("blue")

WINDOW_WIDTH = 1200
WINDOW_HEIGHT = 700
TOOLBOX_WIDTH = 220
STATUS_HEIGHT = 80

# Colors for canvas (matching dark theme)
CANVAS_BG = "#2b2b2b"
CANVAS_GRID = "#3a3a3a"
CANVAS_RAIL = "#888888"

# Ladder element types
ELEMENT_TYPES = [
    ("Contact",       "─| |─",   "I0"),
    ("Not Contact",   "─|/|─",   "I0"),
    ("Coil",          "─( )─",   "Q0"),
    ("Timer (TON)",   "─(TON)─", "T0"),
    ("Counter (CTU)", "─(CTU)─", "C0"),
    ("Reset (RST)",   "─(RST)─", "C0"),
]


# ---------- Main Editor Class ----------
class LadderEditor(ctk.CTk):
    def __init__(self):
        super().__init__()

        self.title("STM32 Mini PLC - Ladder Editor")
        self.geometry(f"{WINDOW_WIDTH}x{WINDOW_HEIGHT}")
        self.minsize(900, 500)

        # State
        self.connected = False
        self.selected_tool = None

        # Build UI
        self._build_layout()
        self._build_toolbox()
        self._build_canvas()
        self._build_status_bar()

    def _build_layout(self):
        # Configure grid
        self.grid_columnconfigure(0, weight=0)  # toolbox - fixed width
        self.grid_columnconfigure(1, weight=1)  # canvas - expand
        self.grid_rowconfigure(0, weight=1)     # main row - expand
        self.grid_rowconfigure(1, weight=0)     # status row - fixed

    def _build_toolbox(self):
        # Toolbox frame
        self.toolbox = ctk.CTkFrame(self, width=TOOLBOX_WIDTH, corner_radius=0)
        self.toolbox.grid(row=0, column=0, sticky="nsew", padx=0, pady=0)
        self.toolbox.grid_propagate(False)

        # Title
        title = ctk.CTkLabel(self.toolbox, text="Toolbox",
                             font=ctk.CTkFont(size=18, weight="bold"))
        title.pack(pady=(20, 10))

        subtitle = ctk.CTkLabel(self.toolbox, text="Click an element, then click the canvas",
                                font=ctk.CTkFont(size=10),
                                text_color="gray")
        subtitle.pack(pady=(0, 20))

        # Element buttons
        self.tool_buttons = {}
        for name, symbol, default_addr in ELEMENT_TYPES:
            btn = ctk.CTkButton(
                self.toolbox,
                text=f"{symbol}\n{name}",
                width=180,
                height=50,
                command=lambda n=name: self._select_tool(n)
            )
            btn.pack(pady=4)
            self.tool_buttons[name] = btn

        # Spacer
        ctk.CTkLabel(self.toolbox, text="").pack(pady=10)

        # Add Rung button
        add_rung_btn = ctk.CTkButton(
            self.toolbox,
            text="+ Add Rung",
            width=180,
            height=40,
            fg_color="#2a7a2a",
            hover_color="#1f5f1f",
            command=self._on_add_rung
        )
        add_rung_btn.pack(pady=4)

        # Clear button
        clear_btn = ctk.CTkButton(
            self.toolbox,
            text="Clear All",
            width=180,
            height=40,
            fg_color="#7a2a2a",
            hover_color="#5f1f1f",
            command=self._on_clear
        )
        clear_btn.pack(pady=4)

    def _build_canvas(self):
        # Canvas frame
        canvas_frame = ctk.CTkFrame(self, corner_radius=0)
        canvas_frame.grid(row=0, column=1, sticky="nsew")
        canvas_frame.grid_columnconfigure(0, weight=1)
        canvas_frame.grid_rowconfigure(0, weight=1)

        # Plain Tkinter Canvas (CustomTkinter doesn't have a canvas)
        self.canvas = Canvas(canvas_frame, bg=CANVAS_BG, highlightthickness=0)
        self.canvas.grid(row=0, column=0, sticky="nsew")
        self.canvas.bind("<Configure>", self._on_canvas_resize)
        self.canvas.bind("<Button-1>", self._on_canvas_click)

    def _build_status_bar(self):
        # Status bar at the bottom (spans both columns)
        self.status_frame = ctk.CTkFrame(self, height=STATUS_HEIGHT, corner_radius=0)
        self.status_frame.grid(row=1, column=0, columnspan=2, sticky="ew")
        self.status_frame.grid_propagate(False)

        # COM port label and dropdown
        ctk.CTkLabel(self.status_frame, text="COM Port:").pack(side="left", padx=(15, 5), pady=20)
        self.com_dropdown = ctk.CTkComboBox(
            self.status_frame,
            values=["COM1", "COM3", "COM9", "Refresh..."],
            width=120
        )
        self.com_dropdown.set("COM9")
        self.com_dropdown.pack(side="left", padx=5)

        # Connect button
        self.connect_btn = ctk.CTkButton(
            self.status_frame,
            text="Connect",
            width=100,
            command=self._on_connect
        )
        self.connect_btn.pack(side="left", padx=10)

        # Compile / Download / Run buttons
        self.compile_btn = ctk.CTkButton(
            self.status_frame, text="Compile", width=100, command=self._on_compile
        )
        self.compile_btn.pack(side="left", padx=5)

        self.download_btn = ctk.CTkButton(
            self.status_frame, text="Download", width=100, command=self._on_download
        )
        self.download_btn.pack(side="left", padx=5)

        # Status label (right side)
        self.status_label = ctk.CTkLabel(
            self.status_frame, text="Status: Not connected",
            text_color="gray"
        )
        self.status_label.pack(side="right", padx=15)

    # ---------- Canvas Drawing ----------
    def _on_canvas_resize(self, event):
        self._redraw_canvas()

    def _redraw_canvas(self):
        self.canvas.delete("all")
        w = self.canvas.winfo_width()
        h = self.canvas.winfo_height()

        # Draw vertical "power rails" — the left and right of every ladder
        rail_x_left = 80
        rail_x_right = w - 80
        self.canvas.create_line(rail_x_left, 30, rail_x_left, h - 30,
                                fill=CANVAS_RAIL, width=3)
        self.canvas.create_line(rail_x_right, 30, rail_x_right, h - 30,
                                fill=CANVAS_RAIL, width=3)

        # Draw subtle grid for visual reference
        for x in range(rail_x_left, rail_x_right, 60):
            self.canvas.create_line(x, 30, x, h - 30, fill=CANVAS_GRID, width=1)

        # Helpful placeholder text
        self.canvas.create_text(
            w / 2, h / 2,
            text="Empty editor\nClick a toolbox element, then click on a rung",
            fill="gray",
            font=("Arial", 14),
            justify="center"
        )

    def _on_canvas_click(self, event):
        if self.selected_tool:
            print(f"[click] x={event.x} y={event.y} tool={self.selected_tool}")
            # In future: place the selected element on the canvas here
        else:
            print(f"[click] x={event.x} y={event.y} (no tool selected)")

    # ---------- Toolbox Actions ----------
    def _select_tool(self, name):
        self.selected_tool = name
        # Highlight selected button — reset others first
        for n, btn in self.tool_buttons.items():
            if n == name:
                btn.configure(fg_color="#1f6aa5")  # darker blue when selected
            else:
                btn.configure(fg_color=("#3a7ebf", "#1f6aa5"))
        print(f"[toolbox] Selected: {name}")
        self.status_label.configure(text=f"Selected: {name}")

    def _on_add_rung(self):
        print("[action] Add rung")

    def _on_clear(self):
        print("[action] Clear all")
        self.selected_tool = None
        for btn in self.tool_buttons.values():
            btn.configure(fg_color=("#3a7ebf", "#1f6aa5"))
        self.status_label.configure(text="Cleared")

    # ---------- Connection / Compile / Download ----------
    def _on_connect(self):
        # Stub - we'll implement real serial connection tomorrow
        if not self.connected:
            self.connected = True
            self.connect_btn.configure(text="Disconnect")
            self.status_label.configure(text=f"Connected to {self.com_dropdown.get()}")
            print(f"[connect] Connected to {self.com_dropdown.get()} (mock)")
        else:
            self.connected = False
            self.connect_btn.configure(text="Connect")
            self.status_label.configure(text="Disconnected")
            print("[connect] Disconnected (mock)")

    def _on_compile(self):
        print("[action] Compile (not yet implemented)")
        self.status_label.configure(text="Compile: not yet implemented")

    def _on_download(self):
        print("[action] Download (not yet implemented)")
        self.status_label.configure(text="Download: not yet implemented")


# ---------- Run the editor ----------
if __name__ == "__main__":
    app = LadderEditor()
    app.mainloop()