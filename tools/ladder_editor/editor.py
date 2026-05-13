"""
STM32 Mini PLC - Ladder Editor
A graphical tool to design ladder programs and download them to the STM32 PLC.
"""

import customtkinter as ctk
from tkinter import Canvas, filedialog, messagebox
import json
import os
import threading
import time
import serial
import serial.tools.list_ports

# ---------- Configuration ----------
ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("blue")

WINDOW_WIDTH = 1200
WINDOW_HEIGHT = 700
TOOLBOX_WIDTH = 220
STATUS_HEIGHT = 80
BAUD = 115200

CANVAS_BG = "#2b2b2b"
CANVAS_GRID = "#3a3a3a"
CANVAS_RAIL = "#888888"
ELEMENT_COLOR = "#5fa8d3"
ELEMENT_ACTIVE = "#5fff5f"
ELEMENT_INACTIVE = "#3a7ebf"
ELEMENT_TEXT = "#ffffff"
ELEMENT_HIT_RADIUS = 35

RAIL_X_LEFT = 80
RAIL_MARGIN_RIGHT = 80
RUNG_HEIGHT = 80
RUNG_TOP_OFFSET = 60
ELEMENT_WIDTH = 80
ELEMENT_HEIGHT = 40
GRID_SNAP = 100
BRANCH_OFFSET_Y = 50

ELEMENT_TYPES = [
    ("Contact",       "─| |─",   "I0", "input"),
    ("Not Contact",   "─|/|─",   "I0", "input"),
    ("Equals",        "─[=]─",   "C0", "equals"),
    ("Coil",          "─( )─",   "Q0", "output"),
    ("Timer (TON)",   "─(TON)─", "T0", "timer"),
    ("Counter (CTU)", "─(CTU)─", "C0", "counter"),
    ("Reset (RST)",   "─(RST)─", "C0", "reset"),
]

VALID_ADDRESSES = {
    "input":   [f"I{i}" for i in range(8)],
    "output":  [f"Q{i}" for i in range(4)],
    "timer":   [f"T{i}" for i in range(8)],
    "counter": [f"C{i}" for i in range(8)],
}
VALID_ADDRESSES["contact"] = (VALID_ADDRESSES["input"] + VALID_ADDRESSES["output"]
                              + [f"T{i}" for i in range(8)]
                              + [f"C{i}" for i in range(8)])
VALID_ADDRESSES["reset"] = VALID_ADDRESSES["timer"] + VALID_ADDRESSES["counter"]
VALID_ADDRESSES["equals"] = (VALID_ADDRESSES["input"] + VALID_ADDRESSES["output"]
                             + [f"T{i}" for i in range(8)]
                             + [f"C{i}" for i in range(8)])


def get_addr_type(elem_type):
    if elem_type in ("Contact", "Not Contact"): return "contact"
    elif elem_type == "Equals": return "equals"
    elif elem_type == "Coil": return "output"
    elif elem_type == "Timer (TON)": return "timer"
    elif elem_type == "Counter (CTU)": return "counter"
    elif elem_type == "Reset (RST)": return "reset"
    return "contact"


OPCODES = {
    "LD": 0x01, "LDN": 0x02, "AND": 0x03, "ANDN": 0x04,
    "OR": 0x05, "ORN": 0x06, "OUT": 0x10,
    "TON": 0x20, "CTU": 0x21,
    "LDT": 0x22, "LDC": 0x23, "LDEQ": 0x24,
    "RST": 0x30, "END": 0xFF,
}


def encode_addr(addr_str):
    if not addr_str: return 0
    prefix = addr_str[0]
    try:
        num = int(addr_str[1:])
    except ValueError:
        return 0
    if prefix == "I": return 0x00 + num
    elif prefix == "Q": return 0x10 + num
    elif prefix == "T": return 0x20 + num
    elif prefix == "C": return 0x30 + num
    return 0


def migrate_program(program):
    if not isinstance(program, dict) or "rungs" not in program:
        return {"rungs": [{"branches": [{"elements": []}]}]}
    for rung in program["rungs"]:
        if "branches" not in rung:
            old_elements = rung.pop("elements", [])
            rung["branches"] = [{"elements": old_elements}]
    return program


# ---------- Element Editor Dialog ----------
class ElementEditorDialog(ctk.CTkToplevel):
    def __init__(self, parent, element):
        super().__init__(parent)
        self.title("Edit Element")
        self.geometry("320x280")
        self.resizable(False, False)
        self.transient(parent)
        self.grab_set()

        self.element = element
        self.result = None

        ctk.CTkLabel(self, text=f"{element['type']}",
                     font=ctk.CTkFont(size=16, weight="bold")).pack(pady=(20, 10))

        ctk.CTkLabel(self, text="Address:").pack(pady=(5, 0))
        addr_type = get_addr_type(element["type"])
        valid = VALID_ADDRESSES.get(addr_type, ["I0"])
        self.addr_var = ctk.StringVar(value=element.get("addr", valid[0]))
        ctk.CTkComboBox(self, values=valid, variable=self.addr_var, width=200).pack(pady=(5, 10))

        self.preset_entry = None
        self.compare_entry = None
        if element["type"] in ("Timer (TON)", "Counter (CTU)"):
            label_text = "Preset (ms)" if element["type"] == "Timer (TON)" else "Preset (count)"
            ctk.CTkLabel(self, text=label_text).pack(pady=(5, 0))
            current_preset = element.get("preset", 1000 if element["type"] == "Timer (TON)" else 5)
            self.preset_entry = ctk.CTkEntry(self, width=200)
            self.preset_entry.insert(0, str(current_preset))
            self.preset_entry.pack(pady=(5, 10))
        elif element["type"] == "Equals":
            ctk.CTkLabel(self, text="Compare value (0-65535)").pack(pady=(5, 0))
            current_value = element.get("value", 0)
            self.compare_entry = ctk.CTkEntry(self, width=200)
            self.compare_entry.insert(0, str(current_value))
            self.compare_entry.pack(pady=(5, 10))

        btn_frame = ctk.CTkFrame(self, fg_color="transparent")
        btn_frame.pack(pady=15)

        ctk.CTkButton(btn_frame, text="OK", width=80, command=self._on_ok).pack(side="left", padx=10)
        ctk.CTkButton(btn_frame, text="Cancel", width=80, fg_color="gray",
                      command=self._on_cancel).pack(side="left", padx=10)
        ctk.CTkButton(btn_frame, text="Delete", width=80,
                      fg_color="#7a2a2a", hover_color="#5f1f1f",
                      command=self._on_delete).pack(side="left", padx=10)

    def _on_ok(self):
        new_addr = self.addr_var.get()
        updated = dict(self.element)
        updated["addr"] = new_addr
        if self.preset_entry is not None:
            try:
                updated["preset"] = int(self.preset_entry.get())
            except ValueError:
                messagebox.showerror("Invalid", "Preset must be a number")
                return
        if self.compare_entry is not None:
            try:
                v = int(self.compare_entry.get())
                if v < 0 or v > 65535:
                    messagebox.showerror("Invalid", "Compare value must be 0-65535")
                    return
                updated["value"] = v
            except ValueError:
                messagebox.showerror("Invalid", "Compare value must be a number")
                return
        self.result = ("update", updated)
        self.destroy()

    def _on_cancel(self):
        self.result = None
        self.destroy()

    def _on_delete(self):
        self.result = ("delete", None)
        self.destroy()


# ---------- Main Editor Class ----------
class LadderEditor(ctk.CTk):
    def __init__(self):
        super().__init__()
        self.title("STM32 Mini PLC - Ladder Editor")
        self.geometry(f"{WINDOW_WIDTH}x{WINDOW_HEIGHT}")
        self.minsize(900, 500)

        self.serial_port = None
        self.serial_lock = threading.Lock()
        self.selected_tool = None
        self.program = {"rungs": [{"branches": [{"elements": []}]}]}
        self.current_file = None

        # Live monitoring state
        self.live_state = {}
        self.live_scan_count = 0
        self.monitoring = False
        self.monitor_thread = None

        self._build_layout()
        self._build_toolbox()
        self._build_canvas()
        self._build_status_bar()
        self._refresh_com_ports()

    def _build_layout(self):
        self.grid_columnconfigure(0, weight=0)
        self.grid_columnconfigure(1, weight=1)
        self.grid_rowconfigure(0, weight=1)
        self.grid_rowconfigure(1, weight=0)

    def _build_toolbox(self):
        self.toolbox = ctk.CTkFrame(self, width=TOOLBOX_WIDTH, corner_radius=0)
        self.toolbox.grid(row=0, column=0, sticky="nsew")
        self.toolbox.grid_propagate(False)

        ctk.CTkLabel(self.toolbox, text="Toolbox",
                     font=ctk.CTkFont(size=18, weight="bold")).pack(pady=(15, 8))

        ctk.CTkLabel(self.toolbox,
                     text="Click element → click rung\nClick to edit, right-click delete\nClick [+B] to add branch",
                     font=ctk.CTkFont(size=10), text_color="gray", justify="left").pack(pady=(0, 10))

        self.tool_buttons = {}
        for name, symbol, default_addr, _ in ELEMENT_TYPES:
            btn = ctk.CTkButton(self.toolbox, text=f"{symbol}\n{name}",
                                width=180, height=36,
                                command=lambda n=name: self._select_tool(n))
            btn.pack(pady=2)
            self.tool_buttons[name] = btn

        ctk.CTkLabel(self.toolbox, text="").pack(pady=2)
        ctk.CTkButton(self.toolbox, text="+ Add Rung", width=180, height=32,
                      fg_color="#2a7a2a", hover_color="#1f5f1f",
                      command=self._on_add_rung).pack(pady=2)
        ctk.CTkButton(self.toolbox, text="Clear All", width=180, height=32,
                      fg_color="#7a2a2a", hover_color="#5f1f1f",
                      command=self._on_clear).pack(pady=2)

        save_load_frame = ctk.CTkFrame(self.toolbox, fg_color="transparent")
        save_load_frame.pack(pady=4)
        ctk.CTkButton(save_load_frame, text="Save", width=85, height=32,
                      command=self._on_save).pack(side="left", padx=4)
        ctk.CTkButton(save_load_frame, text="Load", width=85, height=32,
                      command=self._on_load).pack(side="left", padx=4)

    def _build_canvas(self):
        canvas_frame = ctk.CTkFrame(self, corner_radius=0)
        canvas_frame.grid(row=0, column=1, sticky="nsew")
        canvas_frame.grid_columnconfigure(0, weight=1)
        canvas_frame.grid_rowconfigure(0, weight=1)

        self.canvas = Canvas(canvas_frame, bg=CANVAS_BG, highlightthickness=0)
        self.canvas.grid(row=0, column=0, sticky="nsew")
        self.canvas.bind("<Configure>", lambda e: self._redraw_canvas())
        self.canvas.bind("<Button-1>", self._on_canvas_click)
        self.canvas.bind("<Button-3>", self._on_canvas_right_click)

    def _build_status_bar(self):
        self.status_frame = ctk.CTkFrame(self, height=STATUS_HEIGHT, corner_radius=0)
        self.status_frame.grid(row=1, column=0, columnspan=2, sticky="ew")
        self.status_frame.grid_propagate(False)

        ctk.CTkLabel(self.status_frame, text="COM:").pack(side="left", padx=(15, 5), pady=20)
        self.com_dropdown = ctk.CTkComboBox(self.status_frame, values=["(none)"], width=100)
        self.com_dropdown.pack(side="left", padx=5)

        ctk.CTkButton(self.status_frame, text="↻", width=30,
                      command=self._refresh_com_ports).pack(side="left", padx=(0, 10))

        self.connect_btn = ctk.CTkButton(self.status_frame, text="Connect",
                                         width=100, command=self._on_connect)
        self.connect_btn.pack(side="left", padx=10)

        ctk.CTkButton(self.status_frame, text="Compile", width=90,
                      command=self._on_compile).pack(side="left", padx=4)
        ctk.CTkButton(self.status_frame, text="Download", width=100,
                      fg_color="#2a7a2a", hover_color="#1f5f1f",
                      command=self._on_download).pack(side="left", padx=4)

        # Save-to-EEPROM slot dropdown + button
        ctk.CTkLabel(self.status_frame, text="Slot:").pack(side="left", padx=(10, 2))
        self.slot_var = ctk.StringVar(value="0")
        ctk.CTkComboBox(self.status_frame, values=["0", "1", "2", "3"],
                        variable=self.slot_var, width=55).pack(side="left", padx=2)
        ctk.CTkButton(self.status_frame, text="Save EEPROM", width=120,
                      fg_color="#2a4a7a", hover_color="#1f3a6a",
                      command=self._on_save_to_slot).pack(side="left", padx=4)

        self.monitor_btn = ctk.CTkButton(self.status_frame, text="Monitor: OFF",
                                          width=120, fg_color="#666666",
                                          command=self._on_toggle_monitor)
        self.monitor_btn.pack(side="left", padx=4)

        self.status_label = ctk.CTkLabel(self.status_frame, text="Ready", text_color="gray")
        self.status_label.pack(side="right", padx=15)

    # ============================================================
    # Geometry helpers
    # ============================================================
    def _rung_y(self, rung_index):
        return RUNG_TOP_OFFSET + rung_index * RUNG_HEIGHT

    def _rail_x_right(self):
        return self.canvas.winfo_width() - RAIL_MARGIN_RIGHT

    def _rung_at_y(self, y):
        for i, rung in enumerate(self.program["rungs"]):
            ry = self._rung_y(i)
            n_branches = len(rung["branches"])
            bottom = ry + (n_branches - 1) * BRANCH_OFFSET_Y + 25
            top = ry - RUNG_HEIGHT / 2
            if top <= y <= bottom:
                return i
        return None

    def _snap_x(self, x):
        return round((x - RAIL_X_LEFT) / GRID_SNAP) * GRID_SNAP + RAIL_X_LEFT

    def _find_element_at(self, x, y):
        rung_idx = self._rung_at_y(y)
        if rung_idx is None:
            return None
        rung = self.program["rungs"][rung_idx]
        rung_main_y = self._rung_y(rung_idx)
        for branch_idx, branch in enumerate(rung["branches"]):
            if branch_idx == 0:
                branch_y = rung_main_y
            else:
                branch_y = rung_main_y + branch_idx * BRANCH_OFFSET_Y
            if abs(y - branch_y) <= 25:
                for elem_idx, elem in enumerate(branch["elements"]):
                    if abs(elem["x"] - x) < ELEMENT_HIT_RADIUS:
                        return rung_idx, branch_idx, elem_idx, elem
        return None

    def _branch_at(self, rung_idx, y):
        rung_main_y = self._rung_y(rung_idx)
        rung = self.program["rungs"][rung_idx]
        best_branch = 0
        best_dist = abs(y - rung_main_y)
        for branch_idx in range(1, len(rung["branches"])):
            branch_y = rung_main_y + branch_idx * BRANCH_OFFSET_Y
            d = abs(y - branch_y)
            if d < best_dist:
                best_dist = d
                best_branch = branch_idx
        return best_branch

    def _check_add_branch_click(self, x, y):
        for rung_idx in range(len(self.program["rungs"])):
            ry = self._rung_y(rung_idx)
            bx = RAIL_X_LEFT - 50
            by = ry + 5
            if bx - 18 <= x <= bx + 18 and by - 12 <= y <= by + 12:
                return rung_idx
        return None

    # ============================================================
    # Drawing
    # ============================================================
    def _redraw_canvas(self):
        self.canvas.delete("all")
        w = self.canvas.winfo_width()
        h = self.canvas.winfo_height()
        if w < 100 or h < 100: return

        rail_right = self._rail_x_right()
        bottom_y = self._rung_y(len(self.program["rungs"]) - 1) + RUNG_HEIGHT / 2

        self.canvas.create_line(RAIL_X_LEFT, 30, RAIL_X_LEFT, bottom_y, fill=CANVAS_RAIL, width=3)
        self.canvas.create_line(rail_right, 30, rail_right, bottom_y, fill=CANVAS_RAIL, width=3)

        for rung_idx, rung in enumerate(self.program["rungs"]):
            self._draw_rung(rung_idx, rung)

        all_empty = all(
            all(len(b["elements"]) == 0 for b in r["branches"])
            for r in self.program["rungs"]
        )
        if all_empty:
            self.canvas.create_text(w / 2, h / 2,
                                    text="Pick an element from the toolbox, then click a rung",
                                    fill="gray", font=("Arial", 12))

    def _draw_rung(self, rung_idx, rung):
        y = self._rung_y(rung_idx)
        rail_right = self._rail_x_right()

        self.canvas.create_line(RAIL_X_LEFT, y, rail_right, y, fill=CANVAS_RAIL, width=2)
        self.canvas.create_text(40, y, text=f"R{rung_idx}", fill="gray", font=("Arial", 10))

        bx = RAIL_X_LEFT - 50
        by = y + 5
        self.canvas.create_rectangle(bx - 18, by - 12, bx + 18, by + 12,
                                     fill="#2a4a7a", outline="#5fa8d3", width=1)
        self.canvas.create_text(bx, by, text="+B", fill="white", font=("Arial", 9, "bold"))

        for branch_idx, branch in enumerate(rung["branches"]):
            if branch_idx == 0:
                branch_y = y
            else:
                branch_y = y + branch_idx * BRANCH_OFFSET_Y
                self.canvas.create_line(RAIL_X_LEFT, branch_y, rail_right, branch_y,
                                        fill="#5fa8d3", width=1, dash=(4, 2))
                self.canvas.create_line(RAIL_X_LEFT, y, RAIL_X_LEFT, branch_y,
                                        fill="#5fa8d3", width=1)
                self.canvas.create_line(rail_right, y, rail_right, branch_y,
                                        fill="#5fa8d3", width=1)
                self.canvas.create_text(60, branch_y, text=f"B{branch_idx}",
                                        fill="#5fa8d3", font=("Arial", 9))
            for elem in branch["elements"]:
                self._draw_element(elem, branch_y)

    def _draw_element(self, elem, y):
        x = elem["x"]
        kind = elem["type"]
        addr = elem.get("addr", "?")
        preset = elem.get("preset", None)
        value = elem.get("value", None)

        if self.monitoring:
            is_active = self.live_state.get(addr, False) is True
            if is_active:
                elem_color = ELEMENT_ACTIVE
            else:
                elem_color = ELEMENT_INACTIVE
        else:
            elem_color = ELEMENT_COLOR

        x1 = x - ELEMENT_WIDTH / 2
        x2 = x + ELEMENT_WIDTH / 2
        self.canvas.create_line(x1, y, x2, y, fill=CANVAS_RAIL, width=2)

        if kind == "Contact":
            self.canvas.create_line(x - 12, y - 15, x - 12, y + 15, fill=elem_color, width=3)
            self.canvas.create_line(x + 12, y - 15, x + 12, y + 15, fill=elem_color, width=3)
        elif kind == "Not Contact":
            self.canvas.create_line(x - 12, y - 15, x - 12, y + 15, fill=elem_color, width=3)
            self.canvas.create_line(x + 12, y - 15, x + 12, y + 15, fill=elem_color, width=3)
            self.canvas.create_line(x - 14, y + 14, x + 14, y - 14, fill=elem_color, width=2)
        elif kind == "Coil":
            self.canvas.create_arc(x - 18, y - 18, x + 18, y + 18,
                                   start=45, extent=270, style="arc",
                                   outline=elem_color, width=3)
        elif kind in ("Timer (TON)", "Counter (CTU)", "Reset (RST)"):
            self.canvas.create_rectangle(x - 22, y - 16, x + 22, y + 16,
                                         outline=elem_color, width=2)
            label = {"Timer (TON)": "TON", "Counter (CTU)": "CTU", "Reset (RST)": "RST"}[kind]
            self.canvas.create_text(x, y, text=label,
                                    fill=elem_color, font=("Arial", 10, "bold"))
        elif kind == "Equals":
            self.canvas.create_rectangle(x - 22, y - 16, x + 22, y + 16,
                                         outline=elem_color, width=2)
            self.canvas.create_text(x, y, text="EQ",
                                    fill=elem_color, font=("Arial", 10, "bold"))

        label = addr
        if preset is not None:
            label += f" ={preset}"
        elif value is not None:
            label += f" =={value}"
        self.canvas.create_text(x, y + 28, text=label, fill=ELEMENT_TEXT, font=("Arial", 11, "bold"))

    # ============================================================
    # Canvas interactions
    # ============================================================
    def _on_canvas_click(self, event):
        branch_rung = self._check_add_branch_click(event.x, event.y)
        if branch_rung is not None:
            self.program["rungs"][branch_rung]["branches"].append({"elements": []})
            self.status_label.configure(text=f"Added branch to rung {branch_rung}")
            self._redraw_canvas()
            return

        hit = self._find_element_at(event.x, event.y)
        if hit is not None:
            rung_idx, branch_idx, elem_idx, elem = hit
            self._open_element_editor(rung_idx, branch_idx, elem_idx, elem)
            return

        if not self.selected_tool:
            self.status_label.configure(text="Select a tool first")
            return

        rung_idx = self._rung_at_y(event.y)
        if rung_idx is None:
            self.status_label.configure(text="Click on a rung")
            return

        rail_right = self._rail_x_right()
        if event.x < RAIL_X_LEFT + 30 or event.x > rail_right - 30:
            self.status_label.configure(text="Click between the rails")
            return

        branch_idx = self._branch_at(rung_idx, event.y)

        x = self._snap_x(event.x)
        default_addr = next((addr for n, _, addr, _ in ELEMENT_TYPES if n == self.selected_tool), "?")

        new_element = {"type": self.selected_tool, "addr": default_addr, "x": x}
        if self.selected_tool == "Timer (TON)":
            new_element["preset"] = 2000
        elif self.selected_tool == "Counter (CTU)":
            new_element["preset"] = 5
        elif self.selected_tool == "Equals":
            new_element["value"] = 0

        self.program["rungs"][rung_idx]["branches"][branch_idx]["elements"].append(new_element)
        branch_text = f"branch {branch_idx}" if branch_idx > 0 else "main"
        self.status_label.configure(
            text=f"Placed {self.selected_tool} on rung {rung_idx} ({branch_text})"
        )
        self._redraw_canvas()

    def _on_canvas_right_click(self, event):
        hit = self._find_element_at(event.x, event.y)
        if hit is not None:
            rung_idx, branch_idx, elem_idx, elem = hit
            self.program["rungs"][rung_idx]["branches"][branch_idx]["elements"].pop(elem_idx)
            self.status_label.configure(text=f"Deleted {elem['type']}")
            self._redraw_canvas()

    def _open_element_editor(self, rung_idx, branch_idx, elem_idx, element):
        dialog = ElementEditorDialog(self, element)
        self.wait_window(dialog)
        if dialog.result is None: return
        action, payload = dialog.result
        if action == "update":
            self.program["rungs"][rung_idx]["branches"][branch_idx]["elements"][elem_idx] = payload
            self.status_label.configure(text=f"Updated to {payload['addr']}")
        elif action == "delete":
            self.program["rungs"][rung_idx]["branches"][branch_idx]["elements"].pop(elem_idx)
            self.status_label.configure(text="Deleted")
        self._redraw_canvas()

    # ============================================================
    # Toolbox actions
    # ============================================================
    def _select_tool(self, name):
        self.selected_tool = name
        for n, btn in self.tool_buttons.items():
            if n == name:
                btn.configure(fg_color="#1f6aa5")
            else:
                btn.configure(fg_color=("#3a7ebf", "#1f6aa5"))
        self.status_label.configure(text=f"Selected: {name} — click a rung")

    def _on_add_rung(self):
        self.program["rungs"].append({"branches": [{"elements": []}]})
        self.status_label.configure(text=f"Now {len(self.program['rungs'])} rungs")
        self._redraw_canvas()

    def _on_clear(self):
        self.program = {"rungs": [{"branches": [{"elements": []}]}]}
        self.selected_tool = None
        for btn in self.tool_buttons.values():
            btn.configure(fg_color=("#3a7ebf", "#1f6aa5"))
        self.current_file = None
        self.title("STM32 Mini PLC - Ladder Editor")
        self.status_label.configure(text="Cleared")
        self._redraw_canvas()

    # ============================================================
    # Save / Load JSON files
    # ============================================================
    def _on_save(self):
        path = filedialog.asksaveasfilename(defaultextension=".json",
            filetypes=[("Ladder Program", "*.json")], initialfile="program.json")
        if not path: return
        try:
            with open(path, "w") as f:
                json.dump(self.program, f, indent=2)
            self.current_file = path
            self.title(f"STM32 Mini PLC - Ladder Editor - {os.path.basename(path)}")
            self.status_label.configure(text=f"Saved: {os.path.basename(path)}")
        except Exception as e:
            messagebox.showerror("Save error", str(e))

    def _on_load(self):
        path = filedialog.askopenfilename(filetypes=[("Ladder Program", "*.json")])
        if not path: return
        try:
            with open(path, "r") as f:
                self.program = json.load(f)
            self.program = migrate_program(self.program)
            self.current_file = path
            self.title(f"STM32 Mini PLC - Ladder Editor - {os.path.basename(path)}")
            self.status_label.configure(text=f"Loaded: {os.path.basename(path)}")
            self._redraw_canvas()
        except Exception as e:
            messagebox.showerror("Load error", str(e))

    # ============================================================
    # Compile diagram -> bytecode
    # ============================================================
    def compile_program(self):
        bytecode = []

        for rung_idx, rung in enumerate(self.program["rungs"]):
            branches = rung["branches"]
            if not branches:
                continue

            main_branch = branches[0]
            main_elements = sorted(main_branch["elements"], key=lambda e: e["x"])

            if not main_elements:
                if any(len(b["elements"]) > 0 for b in branches[1:]):
                    return None, f"Rung {rung_idx}: main branch is empty"
                continue

            main_inputs = [e for e in main_elements
                           if e["type"] in ("Contact", "Not Contact", "Equals")]
            outputs = [e for e in main_elements
                       if e["type"] in ("Coil", "Timer (TON)", "Counter (CTU)", "Reset (RST)")]

            if not main_inputs and outputs:
                return None, f"Rung {rung_idx}: outputs need at least one input"

            # Step 1: emit FIRST main input as LD/LDN/LDEQ
            first_input = main_inputs[0]
            addr = encode_addr(first_input["addr"])
            if first_input["type"] == "Contact":
                bytecode.extend([OPCODES["LD"], addr])
            elif first_input["type"] == "Not Contact":
                bytecode.extend([OPCODES["LDN"], addr])
            elif first_input["type"] == "Equals":
                val = first_input.get("value", 0)
                bytecode.extend([OPCODES["LDEQ"], addr, val & 0xFF, (val >> 8) & 0xFF])

            # Step 2: emit all parallel branches as OR/ORN
            for branch_idx in range(1, len(branches)):
                branch = branches[branch_idx]
                has_equals = any(e["type"] == "Equals" for e in branch["elements"])
                if has_equals:
                    return None, (f"Rung {rung_idx}, branch {branch_idx}: "
                                  f"Equals not supported in parallel branches")
                par_elements = [e for e in branch["elements"]
                                if e["type"] in ("Contact", "Not Contact")]
                par_outputs = [e for e in branch["elements"]
                               if e["type"] not in ("Contact", "Not Contact")]

                if not par_elements and not par_outputs:
                    continue
                if par_outputs:
                    return None, (f"Rung {rung_idx}, branch {branch_idx}: "
                                  f"parallel branches can only have contacts")
                if len(par_elements) > 1:
                    return None, (f"Rung {rung_idx}, branch {branch_idx}: "
                                  f"parallel branches must have exactly one element")

                elem = par_elements[0]
                addr = encode_addr(elem["addr"])
                if elem["type"] == "Contact":
                    bytecode.extend([OPCODES["OR"], addr])
                else:
                    bytecode.extend([OPCODES["ORN"], addr])

            # Step 3: emit remaining main inputs as AND/ANDN
            for elem in main_inputs[1:]:
                if elem["type"] == "Equals":
                    return None, ("Equals element must be the first input on a rung")
                addr = encode_addr(elem["addr"])
                if elem["type"] == "Contact":
                    bytecode.extend([OPCODES["AND"], addr])
                else:
                    bytecode.extend([OPCODES["ANDN"], addr])

            # Step 4: emit outputs
            for elem in outputs:
                addr = encode_addr(elem["addr"])
                if elem["type"] == "Coil":
                    bytecode.extend([OPCODES["OUT"], addr])
                elif elem["type"] == "Timer (TON)":
                    preset = elem.get("preset", 1000)
                    bytecode.extend([OPCODES["TON"], addr,
                                     preset & 0xFF, (preset >> 8) & 0xFF])
                elif elem["type"] == "Counter (CTU)":
                    preset = elem.get("preset", 5)
                    bytecode.extend([OPCODES["CTU"], addr,
                                     preset & 0xFF, (preset >> 8) & 0xFF])
                elif elem["type"] == "Reset (RST)":
                    bytecode.extend([OPCODES["RST"], addr])

        bytecode.append(OPCODES["END"])
        return bytecode, None

    def _on_compile(self):
        bytecode, error = self.compile_program()
        if error:
            messagebox.showerror("Compile Error", error)
            self.status_label.configure(text=f"Compile error: {error}")
            return

        print(f"\n=== Bytecode ({len(bytecode)} bytes) ===")
        for i in range(0, len(bytecode), 8):
            chunk = bytecode[i:i+8]
            print(" ".join(f"{b:02X}" for b in chunk))
        print("=====================\n")

        self.status_label.configure(text=f"Compiled: {len(bytecode)} bytes")

    # ============================================================
    # Serial connection
    # ============================================================
    def _refresh_com_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        if not ports:
            ports = ["(none)"]
        self.com_dropdown.configure(values=ports)
        if ports[0] != "(none)":
            self.com_dropdown.set(ports[0])

    def _on_connect(self):
        if self.serial_port and self.serial_port.is_open:
            if self.monitoring:
                self.monitoring = False
                self.monitor_btn.configure(text="Monitor: OFF", fg_color="#666666")
                self.live_state = {}
            self.serial_port.close()
            self.serial_port = None
            self.connect_btn.configure(text="Connect")
            self.status_label.configure(text="Disconnected")
            return

        port = self.com_dropdown.get()
        if port == "(none)":
            messagebox.showerror("No port", "No COM port selected")
            return

        try:
            self.serial_port = serial.Serial(port, BAUD, timeout=2)
            time.sleep(0.5)
            self.connect_btn.configure(text="Disconnect")
            self.status_label.configure(text=f"Connected to {port} @ {BAUD}")
        except Exception as e:
            messagebox.showerror("Connect error", str(e))
            self.serial_port = None

    def _on_download(self):
        if not (self.serial_port and self.serial_port.is_open):
            messagebox.showwarning("Not connected", "Click Connect first")
            return

        bytecode, error = self.compile_program()
        if error:
            messagebox.showerror("Compile Error", error)
            return

        threading.Thread(target=self._download_worker,
                         args=(bytecode,), daemon=True).start()

    def _download_worker(self, bytecode):
        if not self.serial_lock.acquire(timeout=2.0):
            self._set_status("ERROR: serial busy")
            return
        try:
            ser = self.serial_port
            n = len(bytecode)

            self._set_status(f"Uploading {n} bytes...")
            ser.reset_input_buffer()
            ser.write(f"upload {n}\r\n".encode())

            deadline = time.time() + 2.0
            line = b""
            while time.time() < deadline:
                if ser.in_waiting:
                    line += ser.read(ser.in_waiting)
                    if b"READY" in line:
                        break
                time.sleep(0.01)
            else:
                self._set_status("ERROR: STM32 did not say READY")
                return

            ser.write(bytes(bytecode))

            deadline = time.time() + 2.0
            line = b""
            while time.time() < deadline:
                if ser.in_waiting:
                    line += ser.read(ser.in_waiting)
                    if b"OK" in line:
                        break
                time.sleep(0.01)
            else:
                self._set_status("ERROR: STM32 did not confirm bytes")
                return

            self._set_status("Bytes received. Switching...")
            ser.write(b"run\r\n")

            deadline = time.time() + 2.0
            line = b""
            while time.time() < deadline:
                if ser.in_waiting:
                    line += ser.read(ser.in_waiting)
                    if b"RUNNING" in line:
                        break
                time.sleep(0.01)
            else:
                self._set_status("ERROR: STM32 did not confirm run")
                return

            self._set_status(f"✓ Downloaded and running ({n} bytes)")
            print(f"[download] Successfully uploaded and running {n}-byte program")
        except Exception as e:
            self._set_status(f"ERROR: {e}")
            print(f"[download] Exception: {e}")
        finally:
            self.serial_lock.release()

    def _set_status(self, text):
        self.after(0, lambda: self.status_label.configure(text=text))

    # ============================================================
    # Save current program to EEPROM slot on STM32
    # ============================================================
    def _on_save_to_slot(self):
        if not (self.serial_port and self.serial_port.is_open):
            messagebox.showwarning("Not connected", "Connect to STM32 first")
            return
        slot = self.slot_var.get()
        threading.Thread(target=self._save_slot_worker,
                         args=(slot,), daemon=True).start()

    def _save_slot_worker(self, slot):
        try:
            if not self.serial_lock.acquire(timeout=2.0):
                self._set_status("ERROR: serial busy")
                return
            try:
                ser = self.serial_port
                self._set_status(f"Saving to EEPROM slot {slot}...")
                ser.reset_input_buffer()
                ser.write(f"save {slot}\r\n".encode())

                deadline = time.time() + 3.0
                response = b""
                while time.time() < deadline:
                    if ser.in_waiting:
                        response += ser.read(ser.in_waiting)
                        if b"saved" in response.lower() or b"OK" in response:
                            break
                    time.sleep(0.05)

                if b"saved" in response.lower() or b"OK" in response:
                    self._set_status(f"\u2713 Saved to EEPROM slot {slot}")
                    print(f"[save_slot] Slot {slot} saved")
                else:
                    self._set_status(f"ERROR: No confirmation from STM32")
                    print(f"[save_slot] No confirmation. Got: {response}")
            finally:
                self.serial_lock.release()
        except Exception as e:
            self._set_status(f"ERROR: {e}")
            print(f"[save_slot] Exception: {e}")

    # ============================================================
    # Live monitoring
    # ============================================================
    def _on_toggle_monitor(self):
        if self.monitoring:
            self.monitoring = False
            self.monitor_btn.configure(text="Monitor: OFF", fg_color="#666666")
            self.live_state = {}
            self._redraw_canvas()
            self.status_label.configure(text="Monitoring stopped")
            return

        if not (self.serial_port and self.serial_port.is_open):
            messagebox.showwarning("Not connected", "Connect to STM32 first")
            return

        self.monitoring = True
        self.monitor_btn.configure(text="Monitor: ON", fg_color="#c19a2a")
        self.monitor_thread = threading.Thread(target=self._monitor_loop, daemon=True)
        self.monitor_thread.start()

    def _monitor_loop(self):
        while self.monitoring:
            try:
                if not (self.serial_port and self.serial_port.is_open):
                    break

                buf = b""
                if self.serial_lock.acquire(timeout=0.5):
                    try:
                        self.serial_port.reset_input_buffer()
                        time.sleep(0.05)
                        self.serial_port.reset_input_buffer()
                        self.serial_port.write(b"mon\r\n")

                        deadline = time.time() + 0.4
                        while time.time() < deadline:
                            if self.serial_port.in_waiting:
                                buf += self.serial_port.read(self.serial_port.in_waiting)
                                if b"SCAN=" in buf:
                                    time.sleep(0.02)
                                    if self.serial_port.in_waiting:
                                        buf += self.serial_port.read(self.serial_port.in_waiting)
                                    break
                            time.sleep(0.01)
                    finally:
                        self.serial_lock.release()

                state = self._parse_mon(buf.decode("ascii", errors="ignore"))
                if state is not None:
                    self.live_state = state["io"]
                    self.live_scan_count = state["scan"]
                    self.after(0, self._redraw_canvas)
                    scan = self.live_scan_count
                    self.after(0, lambda s=scan: self.status_label.configure(
                        text=f"Live | Scan: {s}"
                    ))

            except Exception as e:
                print(f"[monitor] error: {e}")

            time.sleep(0.2)

    def _parse_mon(self, text):
        # Skip any JSON lines — only look for MON response
        lines = text.split('\n')
        mon_line = None
        for line in lines:
            line = line.strip()
            if line.startswith('MON I='):
                mon_line = line
                break
        
        if mon_line is None:
            return None
        try:
            i_idx = mon_line.find("I=")
            i_part = mon_line[i_idx + 2: i_idx + 10]
            q_idx = mon_line.find("Q=")
            q_part = mon_line[q_idx + 2: q_idx + 6]
            scan_idx = mon_line.find("SCAN=")
            scan_str = mon_line[scan_idx + 5:].strip()

            if len(i_part) < 8 or len(q_part) < 4:
                return None
            if not all(c in "01" for c in i_part):
                return None
            if not all(c in "01" for c in q_part):
                return None

            scan = 0
            try:
                scan = int(scan_str)
            except ValueError:
                pass

            io = {}
            for k in range(8):
                io[f"I{k}"] = (i_part[k] == "1")
            for k in range(4):
                io[f"Q{k}"] = (q_part[k] == "1")
            return {"io": io, "scan": scan}
        except Exception:
            return None


# ---------- Run the editor ----------
if __name__ == "__main__":
    app = LadderEditor()
    app.mainloop()