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
#include <unistd.h>

namespace {
std::string Detab(const std::string &input) {
  std::string output;
  output.reserve(input.size());
  for (char c : input) {
    unsigned char uc = static_cast<unsigned char>(c);
    if (c == '\t') {
      output.append(Edit::TAB_STOP, ' ');
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

  // Handle case where file might be empty or ended with newline
  EnsureLine();
  m_dirty = false;
}

void Buffer::Save() {
  if (m_filename.empty()) {
    throw std::runtime_error("No filename specified");
  }

  // 1. Create temp file
  std::string tempPath = m_filename + Edit::TEMP_EXTENSION;

  // O_CREAT | O_WRONLY | O_TRUNC, 0644 (Owner RW, Group R, Other R)
  int fd = open(tempPath.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
  if (fd < 0) {
    throw std::runtime_error("Failed to create temp file: " +
                             std::string(strerror(errno)));
  }

  // 2. Write content
  try {
    for (size_t i = 0; i < m_lines.size(); ++i) {
      const auto &line = m_lines[i];
      ssize_t written = write(fd, line.c_str(), line.size());
      if (written != (ssize_t)line.size()) {
        throw std::runtime_error("Write failed (incomplete)");
      }

      if (i < m_lines.size() - 1) {
        if (write(fd, "\n", 1) != 1)
          throw std::runtime_error("Write newline failed");
      }
    }

    // 3. Sync to disk
    if (fsync(fd) != 0) {
      throw std::runtime_error("Disk sync failed: " +
                               std::string(strerror(errno)));
    }

    if (close(fd) != 0) {
      throw std::runtime_error("Close failed: " + std::string(strerror(errno)));
    }

    // 4. Atomic Rename
    if (rename(tempPath.c_str(), m_filename.c_str()) != 0) {
      throw std::runtime_error("Atomic rename failed: " +
                               std::string(strerror(errno)));
    }

    m_dirty = false;

  } catch (...) {
    // Cleanup temp file on any failure
    close(fd); // Safe to call even if closed, though errno might be EBADF
    unlink(tempPath.c_str());
    throw; // Re-throw to UI
  }
}

const std::string &Buffer::GetLine(int y) const {
  static const std::string empty = "";
  if (y < 0 || y >= (int)m_lines.size())
    return empty;
  return m_lines[y];
}

int Buffer::LineCount() const { return (int)m_lines.size(); }

bool Buffer::IsDirty() const { return m_dirty; }

void Buffer::InsertChar(int y, int x, int c) {
  if (y < 0 || y >= (int)m_lines.size())
    return;

  // Bounds check x
  if (x < 0)
    x = 0;
  if (x > (int)m_lines[y].size())
    x = (int)m_lines[y].size();

  m_lines[y].insert(x, 1, (char)c);
  m_dirty = true;
}

void Buffer::InsertString(int y, int x, const std::string &str) {
  if (y < 0 || y >= (int)m_lines.size())
    return;

  // Bounds check x
  if (x < 0)
    x = 0;
  if (x > (int)m_lines[y].size())
    x = (int)m_lines[y].size();

  m_lines[y].insert(x, str);
  m_dirty = true;
}

void Buffer::InsertNewLine(int y, int x) {
  if (y < 0 || y >= (int)m_lines.size())
    return;

  if (x < 0)
    x = 0;
  if (x > (int)m_lines[y].size())
    x = (int)m_lines[y].size();

  // Split current line
  std::string current = m_lines[y];
  std::string nextLineContent = current.substr(x);

  // Truncate current
  m_lines[y] = current.substr(0, x);

  // Insert new line after
  m_lines.insert(m_lines.begin() + y + 1, nextLineContent);

  m_dirty = true;
}

void Buffer::DeleteChar(int y, int x) {
  if (y < 0 || y >= (int)m_lines.size())
    return;

  // Case 1: Standard character deletion (backspace within line)
  if (x > 0) {
    size_t prevIdx = TextUtils::PrevCharIdx(m_lines[y], x);
    size_t count = x - prevIdx;

    if (prevIdx < m_lines[y].size()) {
      m_lines[y].erase(prevIdx, count);
      m_dirty = true;
    }
  }
  // Case 2: Line merge (backspace at start of line)
  else if (y > 0) {
    std::string current = m_lines[y];
    std::string prev = m_lines[y - 1];

    m_lines[y - 1] = prev + current;
    m_lines.erase(m_lines.begin() + y);
    m_dirty = true;
  }
}
