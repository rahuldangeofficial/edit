/**
 * @file textutils.hpp
 * @brief UTF-8 text manipulation utilities for visual width calculation and
 * navigation.
 * @author rahuldangeofficial
 */

#ifndef TEXTUTILS_HPP
#define TEXTUTILS_HPP

#include <clocale>
#include <cstdlib>
#include <string>
#include <wchar.h>

namespace TextUtils {

/// Calculate visual width of a string, accounting for multi-column characters.
inline int VisualWidth(const std::string &str) {
  // Locale must be set in main() for mbtowc to work correctly.
  // std::setlocale(LC_ALL, "");

  int width = 0;
  size_t i = 0;
  while (i < str.size()) {
    wchar_t wc;
    int len = mbtowc(&wc, &str[i], str.size() - i);

    if (len < 0) {
      // Invalid UTF-8 sequence, treat as 1-byte, 1-column error char
      mbtowc(NULL, NULL, 0); // reset state
      width += 1;
      i++;
    } else if (len == 0) {
      break; // null terminator
    } else {
      int w = wcwidth(wc);
      width += (w >= 0 ? w : 1); // treat unprintable as size 1 for safety
      i += len;
    }
  }
  return width;
}

/// Get byte length of the UTF-8 character starting at s[i].
inline int CharBytesAt(const std::string &s, size_t i) {
  unsigned char c = static_cast<unsigned char>(s[i]);
  if (c < 0x80)
    return 1;
  if ((c & 0xE0) == 0xC0)
    return 2;
  if ((c & 0xF0) == 0xE0)
    return 3;
  if ((c & 0xF8) == 0xF0)
    return 4;
  return 1; // Fallback for invalid or continuation bytes treated strictly
}

/// Move index forward by one UTF-8 code point.
inline size_t NextCharIdx(const std::string &s, size_t i) {
  if (i >= s.size())
    return s.size();

  // Move forward at least 1 byte
  int len = CharBytesAt(s, i);
  // Standard UTF-8 validation could go here, but for now trusting structure
  // or falling back to safe increments.

  // Ensure we don't land in the middle of a sequence if the string is malformed
  // A primitive loop to skip continuation bytes (0b10xxxxxx)
  size_t next = i + len;
  while (next < s.size()) {
    unsigned char c = static_cast<unsigned char>(s[next]);
    if ((c & 0xC0) != 0x80)
      break; // Not a continuation byte
    next++;
  }

  // If strict length calculation was correct, next should already be correct.
  // But let's be robust:
  return (next > s.size()) ? s.size() : next;
}

/// Move index backward by one UTF-8 code point.
inline size_t PrevCharIdx(const std::string &s, size_t i) {
  if (i == 0)
    return 0;

  // Step back until we find a non-continuation byte
  size_t prev = i - 1;
  while (prev > 0) {
    unsigned char c = static_cast<unsigned char>(s[prev]);
    if ((c & 0xC0) != 0x80)
      break; // Found start of char
    prev--;
  }
  return prev;
}

/// Extract substring starting at visual column offset, fitting within maxCols.
inline std::string TrimToVisual(const std::string &s, int colOff, int maxCols) {
  std::string result;
  int currentVisual = 0;
  size_t i = 0;

  // 1. Advance until colOff visual width is reached
  while (i < s.size() && currentVisual < colOff) {
    wchar_t wc;
    int len = mbtowc(&wc, &s[i], s.size() - i);
    if (len <= 0) {
      i++;
      currentVisual++;
      continue;
    }

    int w = wcwidth(wc);
    if (w < 0)
      w = 1; // Treat unprintable characters as width 1

    currentVisual += w;
    i += len;
  }

  // If split occurs in the middle of a wide character, render from current
  // position.

  // 2. Extract substring fitting within maxCols

  int printedVisual = 0;
  while (i < s.size() && printedVisual < maxCols) {
    wchar_t wc;
    int len = mbtowc(&wc, &s[i], s.size() - i);
    if (len <= 0) {
      result += s[i];
      i++;
      printedVisual++;
      continue;
    }

    int w = wcwidth(wc);
    if (w < 0)
      w = 1;

    if (printedVisual + w > maxCols)
      break; // Don't cut halfway

    result.append(s, i, len);
    printedVisual += w;
    i += len;
  }

  return result;
}

/// Convert a Unicode code point to its UTF-8 encoded string representation.
inline std::string CodePointToUtf8(int cp) {
  std::string result;
  if (cp <= 0x7F) {
    result += (char)cp;
  } else if (cp <= 0x7FF) {
    result += (char)(0xC0 | ((cp >> 6) & 0x1F));
    result += (char)(0x80 | (cp & 0x3F));
  } else if (cp <= 0xFFFF) {
    result += (char)(0xE0 | ((cp >> 12) & 0x0F));
    result += (char)(0x80 | ((cp >> 6) & 0x3F));
    result += (char)(0x80 | (cp & 0x3F));
  } else if (cp <= 0x10FFFF) {
    result += (char)(0xF0 | ((cp >> 18) & 0x07));
    result += (char)(0x80 | ((cp >> 12) & 0x3F));
    result += (char)(0x80 | ((cp >> 6) & 0x3F));
    result += (char)(0x80 | (cp & 0x3F));
  }
  return result;
}

} // namespace TextUtils

#endif // TEXTUTILS_HPP
