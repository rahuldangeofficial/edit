/**
 * @file main.cpp
 * @brief Application entry point for the edit text editor.
 * @author rahuldangeofficial
 * @version 2.0.2
 */

#include "../include/editor.hpp"
#include <clocale>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

// Declare global signal status (also declared in editor.cpp)
extern volatile sig_atomic_t g_signalStatus;
volatile sig_atomic_t g_signalStatus = 0;

/// Signal handler that sets the global status flag.
void SignalHandler(int signal) { g_signalStatus = signal; }

/// Maximum file size before warning (100 MB).
constexpr size_t LARGE_FILE_THRESHOLD = 100 * 1024 * 1024;

/// Check if file appears to be binary (contains null bytes).
bool IsBinaryFile(const std::string &path) {
  int fd = open(path.c_str(), O_RDONLY);
  if (fd < 0) {
    return false; // Can't open, let Load() handle error
  }

  char buf[512];
  ssize_t bytesRead = read(fd, buf, sizeof(buf));
  close(fd);

  if (bytesRead <= 0) {
    return false; // Empty or error, treat as text
  }

  // Check for null bytes (common indicator of binary files)
  for (ssize_t i = 0; i < bytesRead; ++i) {
    if (buf[i] == '\0') {
      return true;
    }
  }
  return false;
}

int main(int argc, char *argv[]) {
  // Set locale for UTF-8 support
  setlocale(LC_ALL, "");

  // Register signal handlers
  signal(SIGINT, SignalHandler);
  signal(SIGTERM, SignalHandler);
#ifdef SIGWINCH
  // Ignore SIGWINCH - ncurses handles terminal resize via getch() returning
  // KEY_RESIZE
  signal(SIGWINCH, SIG_IGN);
#endif

  if (argc != 2) {
    std::cerr << "Usage: edit <filename>" << std::endl;
    return 1;
  }

  std::string path = argv[1];

  // Check file properties before loading
  struct stat st;
  if (stat(path.c_str(), &st) == 0) {
    // Reject directories
    if (S_ISDIR(st.st_mode)) {
      std::cerr << "Error: '" << path << "' is a directory." << std::endl;
      return 1;
    }

    // Check if regular file
    if (S_ISREG(st.st_mode)) {
      // Check read permission
      if (access(path.c_str(), R_OK) != 0) {
        std::cerr << "Error: Cannot read '" << path << "': " << strerror(errno)
                  << std::endl;
        return 1;
      }

      // Check for binary file
      if (IsBinaryFile(path)) {
        std::cerr << "Warning: '" << path << "' appears to be a binary file."
                  << std::endl;
        std::cerr << "Editing binary files may corrupt data. Continue? [y/N] ";
        std::string response;
        std::getline(std::cin, response);
        if (response.empty() || (response[0] != 'y' && response[0] != 'Y')) {
          std::cerr << "Aborted." << std::endl;
          return 0;
        }
      }

      // Check file size
      if (static_cast<size_t>(st.st_size) > LARGE_FILE_THRESHOLD) {
        double sizeMB = static_cast<double>(st.st_size) / (1024 * 1024);
        std::cerr << "Warning: File is " << static_cast<int>(sizeMB) << " MB."
                  << std::endl;
        std::cerr << "Loading large files may be slow. Continue? [y/N] ";
        std::string response;
        std::getline(std::cin, response);
        if (response.empty() || (response[0] != 'y' && response[0] != 'Y')) {
          std::cerr << "Aborted." << std::endl;
          return 0;
        }
      }
    }
  }

  try {
    Editor editor;
    editor.Run(path);

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 2;
  } catch (...) {
    std::cerr << "Error: Unknown Exception" << std::endl;
    return 3;
  }

  return 0;
}
