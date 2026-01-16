/**
 * @file display.cpp
 * @brief Display implementation using ncurses for terminal rendering.
 * @author rahuldangeofficial
 */

#include "../include/display.hpp"
#include "../include/textutils.hpp"
#include <ncurses.h>
#include <stdexcept>
#include <string>

Display::Display() : m_rowOff(0), m_colOff(0), m_gutterWidth(4) {
  // Reduce ESC delay to 25ms for better responsiveness
  setenv("ESCDELAY", "25", 1);

  if (initscr() == NULL) {
    throw std::runtime_error("Failed to initialize ncurses");
  }

  raw();                // Disable line buffering
  noecho();             // Don't echo input
  keypad(stdscr, TRUE); // Enable arrow keys
  timeout(100);         // Non-blocking read (100ms) for signal check
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

  // Horizontal Scroll
  // Convert cursor byte index to visual column
  std::string line = buffer.GetLine(cursorY);
  std::string upToCursor = line.substr(0, cursorX);
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
  // Calculate visual width up to the cursor position
  std::string upToCursor = line.substr(0, cursorX);
  int visualX = TextUtils::VisualWidth(upToCursor);

  move(cursorY - m_rowOff, m_gutterWidth + visualX - m_colOff);
  refresh();
}

void Display::DrawRows(const Buffer &buffer) {
  int maxRows = m_screenRows - 1; // Reserve 1 line for status
  int textAreaWidth = m_screenCols - m_gutterWidth;

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
}

void Display::DrawStatusBar(const Buffer &buffer, int cursorY, int cursorX) {
  attron(A_DIM);

  std::string filename =
      buffer.GetFileName().empty() ? "[No Name]" : buffer.GetFileName();
  std::string details = " - " + std::to_string(buffer.LineCount()) + " lines" +
                        (buffer.IsDirty() ? " (Modified)" : "");

  std::string branding =
      "edit v2.0.0 by @rahuldangeofficial | " + filename + details;

  std::string rStatus = "Ln " + std::to_string(cursorY + 1) + ", Col " +
                        std::to_string(cursorX + 1) + " ";

  int len = (int)branding.size();
  int rLen = (int)rStatus.size();

  // Truncate if screen is too small
  if (len > m_screenCols)
    len = m_screenCols;

  mvprintw(m_screenRows - 1, 0, "%s", branding.substr(0, len).c_str());

  // Fill the rest with whitespace
  for (int i = len; i < m_screenCols; i++) {
    mvaddch(m_screenRows - 1, i, ' ');
  }

  // Right aligned status
  if (m_screenCols > len + rLen) {
    mvprintw(m_screenRows - 1, m_screenCols - rLen, "%s", rStatus.c_str());
  }

  attroff(A_DIM);
}
