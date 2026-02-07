/**
 * @file buffer.cpp
 * @brief Buffer implementation for text storage and file I/O.
 * @author rahuldangeofficial
 */

#include "../include/buffer.hpp"
#include "../include/constants.hpp"
#include "../include/textutils.hpp"
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

namespace {
std::string Detab(const std::string &input) {
  std::string output;
  output.reserve(input.size());
  for (size_t i = 0; i < input.size(); ++i) {
    char c = input[i];
    unsigned char uc = static_cast<unsigned char>(c);
    if (c == '\t') {
      output.append(Edit::TAB_STOP, ' ');
    } else if (c == '\r') {
      // Strip carriage return (handles CRLF files)
      continue;
    } else if (uc >= 32 && uc != 127) {
      // Accept all printable ASCII and all UTF-8 bytes (>= 128)
      output.push_back(c);
    }
  }
  return output;
}
} // namespace

Buffer::Buffer() : m_dirty(false) {
  // Always start with at least one empty line
  m_lines.push_back("");
}

void Buffer::EnsureLine() {
  if (m_lines.empty()) {
    m_lines.push_back("");
  }
}

void Buffer::Load(const std::string &path) {
  m_filename = path;
  m_lines.clear();

  std::ifstream file(path);
  if (!file.is_open()) {
    // New file context, not an error.
    EnsureLine();
    m_dirty = false;
    return;
  }

  std::string line;
  while (std::getline(file, line)) {
    m_lines.push_back(Detab(line));
  }

  // Only add empty line if file was truly empty (no lines read)
  if (m_lines.empty()) {
    m_lines.push_back("");
  }
  m_dirty = false;
}

void Buffer::Save() {
  if (m_filename.empty()) {
    throw std::runtime_error("No filename specified");
  }

  // Get original file permissions (default to 0644 for new files)
  mode_t fileMode = 0644;
  struct stat st;
  if (stat(m_filename.c_str(), &st) == 0) {
    fileMode = st.st_mode & 07777; // Preserve permission bits
  }

  // 1. Create temp file
  std::string tempPath = m_filename + Edit::TEMP_EXTENSION;

  // O_CREAT | O_WRONLY | O_TRUNC with original permissions
  int fd = open(tempPath.c_str(), O_CREAT | O_WRONLY | O_TRUNC, fileMode);
  if (fd < 0) {
    throw std::runtime_error("Failed to create temp file: " +
                             std::string(strerror(errno)));
  }

  // 2. Write content
  try {
    for (size_t i = 0; i < m_lines.size(); ++i) {
      const auto &line = m_lines[i];
      ssize_t written = write(fd, line.c_str(), line.size());
      if (written != static_cast<ssize_t>(line.size())) {
        throw std::runtime_error("Write failed (incomplete)");
      }

      // Add newline after each line (including last - POSIX standard)
      if (write(fd, "\n", 1) != 1)
        throw std::runtime_error("Write newline failed");
    }

    // 3. Sync to disk
    if (fsync(fd) != 0) {
      throw std::runtime_error("Disk sync failed: " +
                               std::string(strerror(errno)));
    }

    if (close(fd) != 0) {
      throw std::runtime_error("Close failed: " + std::string(strerror(errno)));
    }
    fd = -1; // Mark as closed to prevent double-close

    // 4. Atomic Rename
    if (rename(tempPath.c_str(), m_filename.c_str()) != 0) {
      throw std::runtime_error("Atomic rename failed: " +
                               std::string(strerror(errno)));
    }

    m_dirty = false;

  } catch (...) {
    // Cleanup temp file on any failure
    if (fd >= 0) {
      close(fd);
    }
    unlink(tempPath.c_str());
    throw; // Re-throw to UI
  }
}

const std::string &Buffer::GetLine(int y) const {
  static const std::string empty = "";
  if (y < 0 || y >= static_cast<int>(m_lines.size()))
    return empty;
  return m_lines[static_cast<size_t>(y)];
}

int Buffer::LineCount() const { return static_cast<int>(m_lines.size()); }

bool Buffer::IsDirty() const { return m_dirty; }

void Buffer::InsertChar(int y, int x, int c) {
  if (y < 0 || y >= static_cast<int>(m_lines.size()))
    return;

  size_t uy = static_cast<size_t>(y);

  // Bounds check x
  if (x < 0)
    x = 0;
  if (x > static_cast<int>(m_lines[uy].size()))
    x = static_cast<int>(m_lines[uy].size());

  m_lines[uy].insert(static_cast<size_t>(x), 1, static_cast<char>(c));
  m_dirty = true;
}

void Buffer::InsertString(int y, int x, const std::string &str) {
  if (y < 0 || y >= static_cast<int>(m_lines.size()))
    return;

  size_t uy = static_cast<size_t>(y);

  // Bounds check x
  if (x < 0)
    x = 0;
  if (x > static_cast<int>(m_lines[uy].size()))
    x = static_cast<int>(m_lines[uy].size());

  m_lines[uy].insert(static_cast<size_t>(x), str);
  m_dirty = true;
}

void Buffer::InsertNewLine(int y, int x) {
  if (y < 0 || y >= static_cast<int>(m_lines.size()))
    return;

  size_t uy = static_cast<size_t>(y);

  if (x < 0)
    x = 0;
  if (x > static_cast<int>(m_lines[uy].size()))
    x = static_cast<int>(m_lines[uy].size());

  // Split current line
  std::string current = m_lines[uy];
  std::string nextLineContent = current.substr(static_cast<size_t>(x));

  // Truncate current
  m_lines[uy] = current.substr(0, static_cast<size_t>(x));

  // Insert new line after
  m_lines.insert(m_lines.begin() + y + 1, nextLineContent);

  m_dirty = true;
}

void Buffer::DeleteChar(int y, int x) {
  if (y < 0 || y >= static_cast<int>(m_lines.size()))
    return;

  size_t uy = static_cast<size_t>(y);

  // Case 1: Standard character deletion (backspace within line)
  if (x > 0) {
    size_t prevIdx =
        TextUtils::PrevCharIdx(m_lines[uy], static_cast<size_t>(x));
    size_t count = static_cast<size_t>(x) - prevIdx;

    if (prevIdx < m_lines[uy].size()) {
      m_lines[uy].erase(prevIdx, count);
      m_dirty = true;
    }
  }
  // Case 2: Line merge (backspace at start of line)
  else if (y > 0) {
    std::string current = m_lines[uy];
    std::string prev = m_lines[uy - 1];

    m_lines[uy - 1] = prev + current;
    m_lines.erase(m_lines.begin() + y);
    m_dirty = true;
  }
}

void Buffer::DeleteCharForward(int y, int x) {
  if (y < 0 || y >= static_cast<int>(m_lines.size()))
    return;

  size_t uy = static_cast<size_t>(y);
  int lineLen = static_cast<int>(m_lines[uy].size());

  // Case 1: Delete character at cursor position (within line)
  if (x < lineLen) {
    size_t nextIdx =
        TextUtils::NextCharIdx(m_lines[uy], static_cast<size_t>(x));
    size_t count = nextIdx - static_cast<size_t>(x);

    if (x < static_cast<int>(m_lines[uy].size())) {
      m_lines[uy].erase(static_cast<size_t>(x), count);
      m_dirty = true;
    }
  }
  // Case 2: Merge with next line (delete at end of line)
  else if (y < static_cast<int>(m_lines.size()) - 1) {
    std::string current = m_lines[uy];
    std::string next = m_lines[uy + 1];

    m_lines[uy] = current + next;
    m_lines.erase(m_lines.begin() + y + 1);
    m_dirty = true;
  }
}
