## Project: `edit` – The Simplest Terminal Text Editor

### Status: Draft (Initial Planning Phase)

---

### Vision

`edit` is a minimalist, cross-platform, terminal-based text editor designed to solve one practical problem:  
**“I just want to open a file, make quick edits, and exit – without fighting with modes, commands, or keybindings.”**

It's a tool meant for people who:

- Get stuck in Vim trying to edit config files during installation.
- Just want to quickly open and tweak a `.conf`, `.txt`, `.sh`, or `.py` file.
- Work in constrained environments like SSH, Docker, WSL, or minimal Linux distros.
- Need a fast, predictable, and autosaving editor that *just works*.

---

### Core Philosophy

> Simple. Small. Safe.

- **No Plugins**
- **No Language Features**
- **No Formatting**
- **No Learning Curve**
- **No Distractions**

It’s like opening Notepad in the terminal — **type, auto-save, exit**. Nothing more.

---

### Key Goals for P0 Version

1. **Launch with one command**  
   `edit file.txt` — opens or creates the file.

2. **Type and edit directly**  
   Starts in editable mode. No need to press any key to begin.

3. **Autosave**  
   All changes are saved instantly on every keystroke or at small intervals.

4. **Exit easily**  
   Press `Ctrl + Q` to exit cleanly — no need for `:wq`, `ZZ`, or anything complex.

5. **Cross-platform Support**  
   Works on:
    - Ubuntu / Linux (first-class focus)
    - macOS (terminal-based)
    - WSL and MinGW (Windows terminal environments)

---

### Planned Features

| Feature         | Status     |
|-----------------|------------|
| Open & create file | Planned |
| Fullscreen editor (C + ncurses) | Planned |
| Autosave | Planned |
| Basic navigation (arrow keys) | Planned |
| Backspace & new line support | Planned |
| Help screen (`Ctrl + H`) | Optional |
| Scrollable buffer | Later |
| Undo / Redo | Not planned |
| Syntax highlight | Never |

---

### Why It Matters

- **One less barrier** for beginners.
- **One simple tool** you can rely on inside broken servers, SSH sessions, and minimalist containers.
- **Frictionless editing** experience — no panic, no lock-ins, no bloated IDEs.

---

### Future Scope (Optional)

- Add to standard install toolkits like `apt install edit`
- Could be aliased as fallback in broken vim/nano environments
- Bundled with minimal Linux/Unix distros
- Used in repair consoles or embedded shells

---

### Development Notes

This is a personal project. I'm building this to solve a real pain I’ve experienced many times while working on Linux systems, especially when minimal environments are involved and Vim/Nano are overkill.

---

### Current Status

- Idea solidified
- Base structure planned
- C implementation with ncurses coming soon
- Draft version will be updated in this repo progressively

---

### Final Thoughts

There’s power in simplicity.  
Sometimes, the best tool is the one that **stays out of your way**.