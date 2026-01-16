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

    if (m_cy < 0)
      m_cy = 0;
    if (m_cy >= m_buffer.LineCount())
      m_cy = m_buffer.LineCount() - 1;

    int lineLen = (int)m_buffer.GetLine(m_cy).size();
    if (m_cx < 0)
      m_cx = 0;
    if (m_cx > lineLen)
      m_cx = lineLen;

    m_display.Scroll(m_buffer, m_cy, m_cx);
    m_display.Render(m_buffer, m_cy, m_cx);
    ProcessKey();
  }
}

void Editor::ProcessKey() {
  Edit::Key key = Input::ReadKey();

  switch (key.type) {
  case Edit::K_QUIT:
    // Auto-save on quit
    try {
      m_buffer.Save();
    } catch (const std::exception &e) {
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

  default:
    break;
  }
}

void Editor::MoveCursor(int keyType) {
  int rowLen = (int)m_buffer.GetLine(m_cy).size();

  switch (keyType) {
  case Edit::K_ARROW_LEFT:
    if (m_cx > 0) {
      // Move to previous code point
      m_cx = (int)TextUtils::PrevCharIdx(m_buffer.GetLine(m_cy), m_cx);
    } else if (m_cy > 0) {
      m_cy--;
      m_cx = (int)m_buffer.GetLine(m_cy).size();
    }
    break;
  case Edit::K_ARROW_RIGHT:
    if (m_cx < rowLen) {
      // Move to next code point
      m_cx = (int)TextUtils::NextCharIdx(m_buffer.GetLine(m_cy), m_cx);
    } else if (m_cy < m_buffer.LineCount() - 1) {
      m_cy++;
      m_cx = 0;
    }
    break;
  case Edit::K_ARROW_UP:
    if (m_cy > 0)
      m_cy--;
    break;
  case Edit::K_ARROW_DOWN:
    if (m_cy < m_buffer.LineCount() - 1)
      m_cy++;
    break;
  case Edit::K_HOME:
    m_cx = 0;
    break;
  case Edit::K_END:
    m_cx = rowLen;
    break;
  case Edit::K_PAGE_UP:
    m_cy -= m_display.Rows();
    if (m_cy < 0)
      m_cy = 0;
    break;
  case Edit::K_PAGE_DOWN:
    m_cy += m_display.Rows();
    if (m_cy >= m_buffer.LineCount())
      m_cy = m_buffer.LineCount() - 1;
    break;
  }
}

void Editor::InsertChar(int c) {
  if (c < 128) {
    m_buffer.InsertChar(m_cy, m_cx, c);
    m_cx++;
  } else {
    std::string s = TextUtils::CodePointToUtf8(c);
    m_buffer.InsertString(m_cy, m_cx, s);
    m_cx += (int)s.size();
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
    m_buffer.DeleteChar(m_cy, m_cx);
    m_cx--;
  } else {
    // Merge with prev line
    m_cx = (int)m_buffer.GetLine(m_cy - 1).size();
    m_buffer.DeleteChar(m_cy, m_cx); // Logic handled in Buffer
    m_cy--;
  }
}

void Editor::HandleMouseClick(int screenY, int screenX) {
  // Convert screen Y to buffer Y
  int newY = screenY + m_display.GetRowOff();
  if (newY < 0)
    newY = 0;
  if (newY >= m_buffer.LineCount())
    newY = m_buffer.LineCount() - 1;

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
  m_cx = (int)byteX;
}
