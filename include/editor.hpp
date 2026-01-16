/**
 * @file editor.hpp
 * @brief Editor class declaration - main controller orchestrating input,
 * buffer, and display.
 * @author rahuldangeofficial
 */

#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "buffer.hpp"
#include "display.hpp"
#include <string>

/**
 * @class Editor
 * @brief Controller that orchestrates Input, Buffer, and Display.
 *
 * Responsibilities:
 * - Run the main loop.
 * - Dispatch input to modifying actions.
 * - Maintain cursor position.
 *
 * Safety:
 * - Ensures graceful exit.
 * - Exception handling in main loop.
 */
class Editor {
public:
  Editor();
  ~Editor() = default;

  /**
   * @brief Run the editor loop.
   * @param path File to edit.
   */
  void Run(const std::string &path);

private:
  Buffer m_buffer;
  Display m_display; // RAII display

  // Cursor position (0-based)
  int m_cy;
  int m_cx;

  bool m_running;

  // Actions
  void ProcessKey();
  void MoveCursor(int keyType);
  void InsertChar(int c);
  void InsertNewLine();
  void DeleteChar();
  void Save();
  void HandleMouseClick(int screenY, int screenX);
};

#endif // EDITOR_HPP
