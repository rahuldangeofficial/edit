## Project: `edit` – The Simplest Terminal Text Editor

### Status: **Functional – Actively Optimized (P0 Achieved)**

---

### Vision

`edit` is a minimalist, cross-platform, terminal-based text editor designed to solve one practical problem:
**“I just want to open a file, make quick edits, and exit – without fighting with modes, commands, or keybindings.”**

It's a tool meant for people who:

* Get stuck in Vim trying to edit config files during installation.
* Just want to quickly open and tweak a `.conf`, `.txt`, `.sh`, or `.py` file.
* Work in constrained environments like SSH, Docker, WSL, or minimal Linux distros.
* Need a fast, predictable, and autosaving editor that *just works*.

---

### Core Philosophy

> Simple. Small. Safe.

* **No Plugins**
* **No Language Features**
* **No Formatting**
* **No Learning Curve**
* **No Distractions**

It’s like opening Notepad in the terminal — **type, auto-save, exit**. Nothing more.

---

### Key Goals for P0 Version

1. **Launch with one command**
   `edit file.txt` — opens or creates the file.

2. **Type and edit directly**
   Starts in editable mode. No need to press any key to begin.

3. **Autosave**
   All changes are saved instantly on every keystroke (tab-safe and crash-safe).

4. **Exit easily**
   Press `Ctrl + Q` or `ESC` to exit — clean and obvious.

5. **Cross-platform Support**
   Works on:

   * Ubuntu / Linux (first-class focus)
   * macOS (Terminal)
   * WSL / minimal CLI shells

---

### Planned Features

| Feature                                             | Status                           |
| --------------------------------------------------- | -------------------------------- |
| Open & create file                                  | Completed                      |
| Fullscreen editor (C + ncurses)                     | Completed                      |
| Autosave                                            | Completed                      |
| Tab character handling                              | Completed                      |
| Basic navigation (arrow keys)                       | Completed                      |
| Backspace & new line support                        | Completed                      |
| Escape key exits                                    | Completed                      |
| Safe line loading (long lines, tabs, invalid chars) | Completed                      |
| Help screen (`Ctrl + H`)                            | Optional                         |
| Scrollable buffer                                   | Completed (viewport scrolling) |
| Undo / Redo                                         | Not planned                      |
| Syntax highlight                                    | Never                            |

---

### Why It Matters

* **One less barrier** for beginners.
* **One simple tool** you can rely on inside broken servers, SSH sessions, and minimalist containers.
* **Frictionless editing** experience — no panic, no lock-ins, no bloated IDEs.

---

### Future Scope (Optional)

* Submit to official Debian/Ubuntu repos (via `apt install edit`)
* Could act as fallback editor in broken vim/nano systems
* Bundled with recovery consoles or embedded distros
* Could offer read-only or headless viewing mode (log viewer)

---

### Development Notes

This is a personal, utility-focused project built to solve a repeated pain experienced during Linux sysadmin, devops, and low-level work. Built in C with `ncurses`, optimized for performance, stability, and clarity.

---

### Current Work

* Full working `edit.c` completed
* Handles tabs, long lines, cursor alignment, scroll
* Uses atomic save with temp+rename
* Cross-platform tested (macOS and Ubuntu)
* `Makefile` added for build/install/uninstall
* Global install + one-liner documented
* Final output tested inside `/usr/local/bin`
* Global TERM/env bugs fixed with hardcoded fallback

---

### One-line Installer

```bash
git clone https://github.com/rahuldangeofficial/edit.git && cd edit && make install
```

### One-line Uninstaller

```bash
cd edit && make uninstall
```

If you’ve already deleted the repo:

```bash
sudo rm -f /usr/local/bin/edit
```

---

### Usage

```bash
edit file.txt           # Open or create file.txt in fullscreen mode
```

---

### Final Thoughts

There’s power in simplicity.
Sometimes, the best tool is the one that **stays out of your way and never asks for attention**.
`edit` is that tool.

---