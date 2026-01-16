/**
 * @file input.hpp
 * @brief Input handling declarations for keyboard and mouse events.
 * @author rahuldangeofficial
 */

#ifndef INPUT_HPP
#define INPUT_HPP

namespace Edit {

// Key types for internal handling
enum KeyType {
  K_UNKNOWN = 0,
  K_CHAR,
  K_ENTER,
  K_BACKSPACE,
  K_ARROW_UP,
  K_ARROW_DOWN,
  K_ARROW_LEFT,
  K_ARROW_RIGHT,
  K_PAGE_UP,
  K_PAGE_DOWN,
  K_HOME,
  K_END,
  K_DELETE,
  K_ESC,
  K_QUIT, // Ctrl-Q
  K_MOUSE // Mouse click
};

struct Key {
  KeyType type;
  int value;  // ASCII value if type == K_CHAR
  int mouseY; // Screen row if type == K_MOUSE
  int mouseX; // Screen col if type == K_MOUSE
};
} // namespace Edit

/**
 * @class Input
 * @brief Abstracts raw input reading.
 *
 * Responsibilities:
 * - Read character from ncurses.
 * - Translate raw int to internal Key type.
 */
class Input {
public:
  static Edit::Key ReadKey();
};

#endif // INPUT_HPP
