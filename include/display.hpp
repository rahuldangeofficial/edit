/**
 * @file display.hpp
 * @brief Display class declaration for ncurses terminal rendering.
 * @author rahuldangeofficial
 */

#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include "buffer.hpp"

/**
 * @class Display
 * @brief Handles terminal rendering using ncurses.
 *
 * Responsibilities:
 * - Initialize and cleanup ncurses window.
 * - Render visible portion of Buffer.
 * - Render status bar.
 */
class Display {
public:
  Display();
  ~Display();

  /**
   * @brief Render the view.
   * @param buffer The text data.
   * @param cursorY Current cursor Line (0-based in buffer).
   * @param cursorX Current cursor Col (0-based in buffer).
   */
  void Render(const Buffer &buffer, int cursorY, int cursorX);

  /**
   * @brief Update view offsets (scrolling) based on cursor position.
   */
  void Scroll(const Buffer &buffer, int cursorY, int cursorX);

  // Getters for screen dimensions
  int Rows() const;
  int Cols() const;
  int GetRowOff() const;
  int GetColOff() const;
  int GetGutterWidth() const;

private:
  int m_screenRows;
  int m_screenCols;

  // Scrolling offsets (top-left of the view)
  int m_rowOff;
  int m_colOff;

  // Gutter width for line numbers
  int m_gutterWidth;

  void DrawRows(const Buffer &buffer);
  void DrawStatusBar(const Buffer &buffer, int cursorY, int cursorX);
  void UpdateGutterWidth(int lineCount);
};

#endif // DISPLAY_HPP
