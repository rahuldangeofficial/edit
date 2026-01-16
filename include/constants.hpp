/**
 * @file constants.hpp
 * @brief Application-wide constants and configuration values.
 * @author rahuldangeofficial
 */

#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <string>

namespace Edit {

// Helper for safe atomic file operations
const std::string TEMP_EXTENSION = ".tmp";

// UI Defaults
const int TAB_STOP = 4;

// Version Info
const std::string VERSION = "2.0.0";

} // namespace Edit

#endif // CONSTANTS_HPP
