/**
 * @file input.cpp
 * @brief Input handling implementation using ncurses wide character support.
 * @author rahuldangeofficial
 */

#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED
#endif

#include "../include/input.hpp"
#include <ncurses.h>

Edit::Key Input::ReadKey() {
  wint_t ch;
  int ret = get_wch(&ch);

  Edit::Key key = {Edit::K_UNKNOWN, 0, 0, 0};

  // Handle timeout/no input
  if (ret == ERR) {
    key.type = Edit::K_NONE;
    return key;
  }

  if (ret == KEY_CODE_YES) {
    // Handle special keys
    switch (ch) {
    case KEY_UP:
      key.type = Edit::K_ARROW_UP;
      break;
    case KEY_DOWN:
      key.type = Edit::K_ARROW_DOWN;
      break;
    case KEY_LEFT:
      key.type = Edit::K_ARROW_LEFT;
      break;
    case KEY_RIGHT:
      key.type = Edit::K_ARROW_RIGHT;
      break;
    case KEY_HOME:
      key.type = Edit::K_HOME;
      break;
    case KEY_END:
      key.type = Edit::K_END;
      break;
    case KEY_PPAGE:
      key.type = Edit::K_PAGE_UP;
      break;
    case KEY_NPAGE:
      key.type = Edit::K_PAGE_DOWN;
      break;
    case KEY_DC:
      key.type = Edit::K_DELETE;
      break;
    case KEY_BACKSPACE:
      key.type = Edit::K_BACKSPACE;
      break;
    case KEY_MOUSE: {
      MEVENT event;
      if (getmouse(&event) == OK) {
        key.type = Edit::K_MOUSE;
        key.mouseY = event.y;
        key.mouseX = event.x;
      }
      break;
    }
    case KEY_RESIZE:
      key.type = Edit::K_RESIZE;
      break;
    default:
      break;
    }
  } else if (ret == OK) {
    // Handle standard characters and control codes
    switch (ch) {
    case 127:
    case 8:
      key.type = Edit::K_BACKSPACE;
      break;
    case '\n':
    case '\r':
      key.type = Edit::K_ENTER;
      break;
    case 27: // ESC only
      key.type = Edit::K_QUIT;
      break;
    default:
      // Allow tab and printable characters (including Unicode)
      if (ch == '\t' || ch >= 32) {
        key.type = Edit::K_CHAR;
        key.value = static_cast<int>(ch);
      }
      // Control chars 0-31 (except tab) are ignored
      break;
    }
  }

  return key;
}
