/**
 * @file display.cpp
 * @brief Display implementation using ncurses for terminal rendering.
 * @author rahuldangeofficial
 */

#include "../include/display.hpp"
#include "../include/constants.hpp"
#include "../include/textutils.hpp"
#include <ncurses.h>
#include <stdexcept>
#include <string>

Display::Display() : m_rowOff(0), m_colOff(0), m_gutterWidth(4) {
  // Reduce ESC delay to minimum for instant response
  setenv("ESCDELAY", "5", 1);

  if (initscr() == NULL) {
    throw std::runtime_error("Failed to initialize ncurses");
  }

  raw();                            // Disable line buffering
  noecho();                         // Don't echo input
  keypad(stdscr, TRUE);             // Enable arrow keys
  timeout(16);                      // ~60fps, minimal delay
  mousemask(BUTTON1_CLICKED, NULL); // Enable left-click

  getmaxyx(stdscr, m_screenRows, m_screenCols);

  if (m_screenRows <= 0 || m_screenCols <= 0) {
    endwin();
    throw std::runtime_error("Terminal too small");
  }
}

Display::~Display() {
  // RAII: Always clean up terminal state
  endwin();
}

int Display::Rows() const { return m_screenRows; }
int Display::Cols() const { return m_screenCols; }
int Display::GetRowOff() const { return m_rowOff; }
int Display::GetColOff() const { return m_colOff; }
int Display::GetGutterWidth() const { return m_gutterWidth; }

void Display::Scroll(const Buffer &buffer, int cursorY, int cursorX) {
  m_screenRows = getmaxy(stdscr);
  m_screenCols = getmaxx(stdscr);

  // Update gutter width based on line count
  UpdateGutterWidth(buffer.LineCount());

  // Vertical Scroll
  if (cursorY < m_rowOff) {
    m_rowOff = cursorY;
  }
  if (cursorY >= m_rowOff + m_screenRows - 1) { // -1 for status bar
    m_rowOff = cursorY - m_screenRows + 2;
  }
  // Ensure scroll offset is never negative
  if (m_rowOff < 0) {
    m_rowOff = 0;
  }

  // Horizontal Scroll
  // Convert cursor byte index to visual column
  std::string line = buffer.GetLine(cursorY);
  int lineSize = static_cast<int>(line.size());
  int safeX = (cursorX > lineSize) ? lineSize : cursorX;
  std::string upToCursor = line.substr(0, static_cast<size_t>(safeX));
  int visualX = TextUtils::VisualWidth(upToCursor);

  int textAreaWidth = m_screenCols - m_gutterWidth;
  if (visualX < m_colOff) {
    m_colOff = visualX;
  }
  if (visualX >= m_colOff + textAreaWidth) {
    m_colOff = visualX - textAreaWidth + 1;
  }
}

void Display::Render(const Buffer &buffer, int cursorY, int cursorX) {
  erase();
  DrawRows(buffer);
  DrawStatusBar(buffer, cursorY, cursorX);

  // Map byte-index cursor to visual column
  std::string line = buffer.GetLine(cursorY);
  // Calculate visual width up to the cursor position (with bounds check)
  int lineSize = static_cast<int>(line.size());
  int safeX = (cursorX > lineSize) ? lineSize : cursorX;
  std::string upToCursor = line.substr(0, static_cast<size_t>(safeX));
  int visualX = TextUtils::VisualWidth(upToCursor);

  // Clamp cursor screen position to valid range
  int screenY = cursorY - m_rowOff;
  int screenX = m_gutterWidth + visualX - m_colOff;
  if (screenY < 0)
    screenY = 0;
  if (screenY >= m_screenRows - 1)
    screenY = m_screenRows - 2;
  if (screenX < m_gutterWidth)
    screenX = m_gutterWidth;
  if (screenX >= m_screenCols)
    screenX = m_screenCols - 1;

  move(screenY, screenX);
  refresh();
}

void Display::DrawRows(const Buffer &buffer) {
  int maxRows = m_screenRows - 1; // Reserve 1 line for status
  int textAreaWidth = m_screenCols - m_gutterWidth;

  // Safety check for very narrow terminals
  if (textAreaWidth < 1) {
    textAreaWidth = 1;
  }

  for (int y = 0; y < maxRows; y++) {
    int fileRow = y + m_rowOff;

    // Draw gutter background for all rows
    attron(A_DIM);
    for (int i = 0; i < m_gutterWidth; i++) {
      mvaddch(y, i, ' ');
    }
    attroff(A_DIM);

    if (fileRow >= buffer.LineCount()) {
      // Empty line beyond file - gutter already drawn, leave text area blank
      continue;
    }

    // Draw line number (right-aligned in gutter)
    attron(A_DIM);
    mvprintw(y, 0, "%*d ", m_gutterWidth - 1, fileRow + 1);
    attroff(A_DIM);

    const std::string &line = buffer.GetLine(fileRow);

    // Trim string to visual width
    std::string printLine =
        TextUtils::TrimToVisual(line, m_colOff, textAreaWidth);

    if (!printLine.empty()) {
      mvaddstr(y, m_gutterWidth, printLine.c_str());
    }
  }
}

void Display::UpdateGutterWidth(int lineCount) {
  // Calculate digits needed for max line number + 1 space
  int digits = 1;
  int n = lineCount;
  while (n >= 10) {
    n /= 10;
    digits++;
  }
  m_gutterWidth = digits + 1; // +1 for space separator

  // Enforce minimum gutter width of 2
  if (m_gutterWidth < 2) {
    m_gutterWidth = 2;
  }
}

void Display::DrawStatusBar(const Buffer &buffer, int cursorY, int cursorX) {
  attron(A_DIM);

  std::string filename =
      buffer.GetFileName().empty() ? "[No Name]" : buffer.GetFileName();
  std::string modified = buffer.IsDirty() ? "*" : "";
  std::string lineInfo = std::to_string(buffer.LineCount()) + "L";

  // Calculate visual column
  const std::string &line = buffer.GetLine(cursorY);
  int lineSize = static_cast<int>(line.size());
  int safeX = (cursorX > lineSize) ? lineSize : cursorX;
  std::string upToCursor = line.substr(0, static_cast<size_t>(safeX));
  int visualCol = TextUtils::VisualWidth(upToCursor) + 1;
  std::string cursorInfo =
      std::to_string(cursorY + 1) + ":" + std::to_string(visualCol);

  int row = m_screenRows - 1;

  // Progressive truncation levels
  std::string fullBrand = "edit v" + Edit::VERSION + " by @rahuldangeofficial";
  std::string shortBrand = "edit v" + Edit::VERSION;
  std::string minBrand = "edit";
  std::string fileStr = filename + modified + " " + lineInfo;
  std::string right = "ESC | " + cursorInfo;

  std::string left;
  int gap = 3; // minimum gap between left and right

  // Calculate sizes as int
  int fullLen = static_cast<int>(fullBrand.size() + 3 + fileStr.size()) + gap +
                static_cast<int>(right.size());
  int shortLen = static_cast<int>(shortBrand.size() + 3 + fileStr.size()) +
                 gap + static_cast<int>(right.size());
  int minLen = static_cast<int>(minBrand.size() + 3 + fileStr.size()) + gap +
               static_cast<int>(right.size());
  int fileOnlyLen =
      static_cast<int>(fileStr.size()) + gap + static_cast<int>(right.size());

  // Try progressively shorter versions
  if (fullLen <= m_screenCols) {
    left = fullBrand + " | " + fileStr;
  } else if (shortLen <= m_screenCols) {
    left = shortBrand + " | " + fileStr;
  } else if (minLen <= m_screenCols) {
    left = minBrand + " | " + fileStr;
  } else if (fileOnlyLen <= m_screenCols) {
    left = fileStr;
  } else {
    // Very narrow: truncate filename
    int avail = m_screenCols - static_cast<int>(right.size()) - gap - 2;
    if (avail > 3 && avail <= static_cast<int>(filename.size())) {
      left = filename.substr(0, static_cast<size_t>(avail)) + "..";
    } else {
      left = "";
    }
  }

  // Draw left side
  mvprintw(row, 0, "%s", left.c_str());

  // Fill with spaces
  int leftLen = static_cast<int>(left.size());
  for (int i = leftLen; i < m_screenCols; i++) {
    mvaddch(row, i, ' ');
  }

  // Draw right side
  int rightLen = static_cast<int>(right.size());
  if (m_screenCols > rightLen) {
    mvprintw(row, m_screenCols - rightLen, "%s", right.c_str());
  }

  attroff(A_DIM);
}
