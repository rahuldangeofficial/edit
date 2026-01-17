# edit v2.0.0 by @rahuldangeofficial

A minimalist terminal text editor. Zero-config, UTF-8 native, atomically safe.

---

## Performance Comparison

| Metric | edit | nano | vim | micro |
|--------|------|------|-----|-------|
| Binary Size | 64 KB | ~200 KB | 5.4 MB | 11 MB |
| RAM Usage | ~1-2 MB | ~3 MB | ~10 MB | ~30 MB |
| Startup Time | Instant | Fast | Slow | Moderate |
| Full UTF-8/Emoji | Yes | Partial | Yes | Yes |
| Atomic Save | Yes | No | No | No |
| Crash Recovery | Yes | No | Swap file | No |

---

## Features

- **64 KB binary** — 84x smaller than vim
- **Atomic saves** — write, fsync, rename (no data corruption)
- **Crash-safe** — Ctrl+C triggers save before exit
- **True UTF-8** — Emojis render and save correctly
- **Line numbers** — Always visible, dynamic width
- **Mouse support** — Click to position cursor
- **Large file warning** — Prompts before loading files >100 MB

---

## Installation

**One-liner install (auto-installs dependencies):**
```bash
git clone https://github.com/rahuldangeofficial/edit.git && cd edit && bash install.sh
```

**Manual install (if you have dependencies):**
```bash
git clone https://github.com/rahuldangeofficial/edit.git && cd edit && make && sudo make install
```

**Uninstall:**
```bash
cd edit && sudo make uninstall
```

### Supported Platforms
- Debian / Ubuntu
- Fedora / RHEL / CentOS
- Arch Linux
- Alpine Linux
- macOS

---

## Usage

```bash
edit filename.txt
```

### Controls

| Key | Action |
|-----|--------|
| Arrow keys | Navigate |
| Home / End | Jump to line start/end |
| PageUp / PageDown | Scroll |
| Backspace | Delete character |
| Enter | New line |
| Esc / Ctrl+Q | Save and exit |
| Mouse click | Position cursor |

---

## Design Philosophy

edit is intentionally minimal. It does not include:

- Syntax highlighting
- Undo/redo
- Search
- Plugins
- Config files

This is by design. edit is for quick edits, not IDE workflows.

---

## Use Cases

- Config file edits (~/.bashrc, ~/.gitconfig)
- Git commit messages
- Small scripts and notes
- Embedded systems / Docker containers
- Low-RAM environments (Raspberry Pi, VPS)

---

## Requirements

- C++17 compiler (g++, clang++)
- ncurses library
- POSIX system (Linux, macOS)

---

## License

MIT