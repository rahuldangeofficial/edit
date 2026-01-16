/**
 * @file buffer.hpp
 * @brief Buffer class declaration for text storage and file I/O.
 * @author rahuldangeofficial
 */

#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <string>
#include <vector>

/**
 * @class Buffer
 * @brief Manages the text content of the file being edited.
 *
 * Responsibilities:
 * - Stores lines of text strings in a vector.
 * - Handles file I/O operations (Load, Save).
 * - Implements modifications (Insert, Delete).
 * - Tracks "dirty" state (unsaved changes).
 *
 * Safety:
 * - All indices are bounds-checked.
 * - File operations use robust error handling.
 * - "Atomic Save" guarantees no data corruption during write.
 */
class Buffer {
public:
  Buffer();
  ~Buffer() = default;

  /**
   * @brief Load content from a file path.
   * @param path File path.
   * @throws std::runtime_error if file exisits but cannot be read.
   */
  void Load(const std::string &path);

  /**
   * @brief Save content to disk atomically.
   *
   * Strategy:
   * 1. Write to {filename}.tmp.
   * 2. fsync() to ensure data hits the disk.
   * 3. Rename {filename}.tmp to {filename} (POSIX atomic guarantee).
   *
   * @throws std::runtime_error on I/O failure.
   */
  void Save();

  // --- content access ---

  /**
   * @brief Get a read-only reference to a specific line.
   * @param y 0-based line index.
   * @return const std::string& line content.
   * @note Returns empty string if out of bounds (safe access).
   */
  const std::string &GetLine(int y) const;

  /**
   * @brief Get total number of lines.
   */
  int LineCount() const;

  /**
   * @brief Check if buffer has unsaved changes.
   */
  bool IsDirty() const;

  // --- modification ---

  /**
   * @brief Insert a character at specific coordinates.
   * @param y Line index.
   * @param x Column index.
   * @param c Character to insert.
   */
  void InsertChar(int y, int x, int c);

  /**
   * @brief Insert a string at specific coordinates.
   * @param y Line index.
   * @param x Column index.
   * @param str String to insert.
   */
  void InsertString(int y, int x, const std::string &str);

  /**
   * @brief Delete character at specific coordinates.
   * @param y Line index.
   * @param x Column index.
   */
  void DeleteChar(int y, int x);

  /**
   * @brief Insert a newline at coordinates (split line).
   * @param y Line index.
   * @param x Column index (split point).
   */
  void InsertNewLine(int y, int x);

  // --- helpers ---

  const std::string &GetFileName() const { return m_filename; }

private:
  std::vector<std::string> m_lines;
  std::string m_filename;
  bool m_dirty;

  // Ensure at least one line exists
  void EnsureLine();
};

#endif // BUFFER_HPP
