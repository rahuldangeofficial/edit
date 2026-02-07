/**
 * @file editor.cpp
 * @brief Editor implementation - main loop and input dispatch.
 * @author rahuldangeofficial
 */

#include "../include/editor.hpp"
#include "../include/constants.hpp"
#include "../include/input.hpp"
#include "../include/textutils.hpp"
#include <exception>
#include <signal.h>
#include <unistd.h>

extern volatile sig_atomic_t g_signalStatus;

namespace {
/// Snap cursor X to valid UTF-8 boundary on the given line
void SnapCursorToLine(int &cx, const std::string &line) {
  int lineLen = static_cast<int>(line.size());
  if (cx > lineLen) {
    cx = lineLen;
  } else if (cx > 0 && cx < lineLen) {
    // Ensure we're not in middle of UTF-8 sequence
    size_t pos = static_cast<size_t>(cx);
    unsigned char c = static_cast<unsigned char>(line[pos]);
    while (cx > 0 && (c & 0xC0) == 0x80) {
      cx--;
      pos = static_cast<size_t>(cx);
      c = static_cast<unsigned char>(line[pos]);
    }
  }
}
} // namespace

Editor::Editor() : m_cy(0), m_cx(0), m_running(false) {}

void Editor::Run(const std::string &path) {
  m_buffer.Load(path);
  m_running = true;

  while (m_running) {
    // Check for external signal (Ctrl+C etc)
    if (g_signalStatus != 0) {
      try {
        m_buffer.Save();
      } catch (...) {
        // Best effort save
      }
      m_running = false;
      break;
    }

    // Cursor bounds safety (with empty buffer protection)
    int lineCount = m_buffer.LineCount();
    if (lineCount == 0) {
      m_cy = 0;
      m_cx = 0;
    } else {
      if (m_cy < 0)
        m_cy = 0;
      if (m_cy >= lineCount)
        m_cy = lineCount - 1;

      int lineLen = static_cast<int>(m_buffer.GetLine(m_cy).size());
      if (m_cx < 0)
        m_cx = 0;
      if (m_cx > lineLen)
        m_cx = lineLen;
    }

    m_display.Scroll(m_buffer, m_cy, m_cx);
    m_display.Render(m_buffer, m_cy, m_cx);
    ProcessKey();
  }
}

void Editor::ProcessKey() {
  Edit::Key key = Input::ReadKey();

  switch (key.type) {
  case Edit::K_UNKNOWN:
  case Edit::K_NONE:
    // Timeout or unknown, no action needed
    break;
  default:
    break;

  case Edit::K_QUIT:
    // Auto-save on quit
    try {
      m_buffer.Save();
    } catch (const std::exception &) {
      // Exceptions propagate to main for reporting
      throw;
    }
    m_running = false;
    break;

  case Edit::K_CHAR:
    if (key.value == '\t') {
      for (int i = 0; i < Edit::TAB_STOP; ++i)
        InsertChar(' ');
    } else {
      InsertChar(key.value);
    }
    break;

  case Edit::K_ENTER:
    InsertNewLine();
    break;

  case Edit::K_BACKSPACE:
    DeleteChar();
    break;

  case Edit::K_DELETE:
    DeleteCharForward();
    break;

  case Edit::K_ARROW_UP:
  case Edit::K_ARROW_DOWN:
  case Edit::K_ARROW_LEFT:
  case Edit::K_ARROW_RIGHT:
  case Edit::K_HOME:
  case Edit::K_END:
  case Edit::K_PAGE_UP:
  case Edit::K_PAGE_DOWN:
    MoveCursor(key.type);
    break;

  case Edit::K_MOUSE:
    HandleMouseClick(key.mouseY, key.mouseX);
    break;

  case Edit::K_RESIZE:
    // Terminal resized - no action needed, Scroll/Render will pick up new size
    break;
  }
}

void Editor::MoveCursor(int keyType) {
  int rowLen = static_cast<int>(m_buffer.GetLine(m_cy).size());

  switch (keyType) {
  default:
    break;
  case Edit::K_ARROW_LEFT:
    if (m_cx > 0) {
      // Move to previous code point
      m_cx = static_cast<int>(TextUtils::PrevCharIdx(
          m_buffer.GetLine(m_cy), static_cast<size_t>(m_cx)));
    } else if (m_cy > 0) {
      m_cy--;
      m_cx = static_cast<int>(m_buffer.GetLine(m_cy).size());
    }
    break;
  case Edit::K_ARROW_RIGHT: {
    int lineCount = m_buffer.LineCount();
    if (m_cx < rowLen) {
      // Move to next code point
      m_cx = static_cast<int>(TextUtils::NextCharIdx(
          m_buffer.GetLine(m_cy), static_cast<size_t>(m_cx)));
    } else if (lineCount > 0 && m_cy < lineCount - 1) {
      m_cy++;
      m_cx = 0;
    }
    break;
  }
  case Edit::K_ARROW_UP:
    if (m_cy > 0) {
      m_cy--;
      SnapCursorToLine(m_cx, m_buffer.GetLine(m_cy));
    }
    break;
  case Edit::K_ARROW_DOWN: {
    int lineCount = m_buffer.LineCount();
    if (lineCount > 0 && m_cy < lineCount - 1) {
      m_cy++;
      SnapCursorToLine(m_cx, m_buffer.GetLine(m_cy));
    }
    break;
  }
  case Edit::K_HOME:
    m_cx = 0;
    break;
  case Edit::K_END:
    m_cx = rowLen;
    break;
  case Edit::K_PAGE_UP: {
    m_cy -= m_display.Rows();
    if (m_cy < 0)
      m_cy = 0;
    SnapCursorToLine(m_cx, m_buffer.GetLine(m_cy));
    break;
  }
  case Edit::K_PAGE_DOWN: {
    int lineCount = m_buffer.LineCount();
    m_cy += m_display.Rows();
    if (lineCount > 0 && m_cy >= lineCount)
      m_cy = lineCount - 1;
    else if (lineCount == 0)
      m_cy = 0;
    SnapCursorToLine(m_cx, m_buffer.GetLine(m_cy));
    break;
  }
  }
}

void Editor::InsertChar(int c) {
  if (c < 128) {
    m_buffer.InsertChar(m_cy, m_cx, c);
    m_cx++;
  } else {
    std::string s = TextUtils::CodePointToUtf8(c);
    m_buffer.InsertString(m_cy, m_cx, s);
    m_cx += static_cast<int>(s.size());
  }
}

void Editor::InsertNewLine() {
  m_buffer.InsertNewLine(m_cy, m_cx);
  m_cy++;
  m_cx = 0;
}

void Editor::DeleteChar() {
  if (m_cy == 0 && m_cx == 0)
    return;

  if (m_cx > 0) {
    // Calculate new cursor position before deletion (UTF-8 aware)
    int newCx = static_cast<int>(TextUtils::PrevCharIdx(
        m_buffer.GetLine(m_cy), static_cast<size_t>(m_cx)));
    m_buffer.DeleteChar(m_cy, m_cx);
    m_cx = newCx;
  } else {
    // Merge with prev line: pass x=0 since cursor is at start of line
    int prevLineLen = static_cast<int>(m_buffer.GetLine(m_cy - 1).size());
    m_buffer.DeleteChar(m_cy, 0);
    m_cx = prevLineLen;
    m_cy--;
  }
}

void Editor::DeleteCharForward() {
  int lineCount = m_buffer.LineCount();

  // Safety check for empty buffer
  if (lineCount == 0)
    return;

  int lineLen = static_cast<int>(m_buffer.GetLine(m_cy).size());

  // At end of last line, nothing to delete forward
  if (m_cy == lineCount - 1 && m_cx >= lineLen)
    return;

  // Delete character at cursor or merge with next line
  m_buffer.DeleteCharForward(m_cy, m_cx);
  // Cursor position stays the same for forward delete
}

void Editor::HandleMouseClick(int screenY, int screenX) {
  int lineCount = m_buffer.LineCount();

  // Safety check for empty buffer
  if (lineCount == 0) {
    m_cy = 0;
    m_cx = 0;
    return;
  }

  // Convert screen Y to buffer Y
  int newY = screenY + m_display.GetRowOff();
  if (newY < 0)
    newY = 0;
  if (newY >= lineCount)
    newY = lineCount - 1;

  m_cy = newY;

  // Convert screen X to buffer X (accounting for gutter and visual width)
  int gutterWidth = m_display.GetGutterWidth();
  int visualX = screenX - gutterWidth + m_display.GetColOff();
  if (visualX < 0)
    visualX = 0;

  // Translate visual X to byte X
  const std::string &line = m_buffer.GetLine(m_cy);
  size_t byteX = 0;
  int currentVisual = 0;
  while (byteX < line.size() && currentVisual < visualX) {
    size_t nextByte = TextUtils::NextCharIdx(line, byteX);
    std::string charStr = line.substr(byteX, nextByte - byteX);
    currentVisual += TextUtils::VisualWidth(charStr);
    byteX = nextByte;
  }

  // Snap to valid UTF-8 boundary (in case we landed mid-sequence)
  if (byteX > 0 && byteX < line.size()) {
    unsigned char c = static_cast<unsigned char>(line[byteX]);
    while (byteX > 0 && (c & 0xC0) == 0x80) {
      byteX--;
      c = static_cast<unsigned char>(line[byteX]);
    }
  }

  m_cx = static_cast<int>(byteX);
}
