/**
 * @file main.cpp
 * @brief Application entry point for the edit text editor.
 * @author rahuldangeofficial
 * @version 2.0.0
 */

#include "../include/editor.hpp"
#include <clocale>
#include <iostream>
#include <signal.h>
#include <sys/stat.h>

/// Global signal status for graceful shutdown handling.
volatile sig_atomic_t g_signalStatus = 0;

/// Signal handler that sets the global status flag.
void SignalHandler(int signal) { g_signalStatus = signal; }

/// Maximum file size before warning (100 MB).
constexpr size_t LARGE_FILE_THRESHOLD = 100 * 1024 * 1024;

int main(int argc, char *argv[]) {
  // Set locale for UTF-8 support
  setlocale(LC_ALL, "");

  // Register signal handler (though ncurses mode makes this tricky to use
  // safely)
  signal(SIGINT, SignalHandler);
  signal(SIGTERM, SignalHandler);

  if (argc != 2) {
    std::cerr << "Usage: edit <filename>" << std::endl;
    return 1;
  }

  std::string path = argv[1];

  // Check file size before loading
  struct stat st;
  if (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
    if ((size_t)st.st_size > LARGE_FILE_THRESHOLD) {
      double sizeMB = (double)st.st_size / (1024 * 1024);
      std::cerr << "Warning: File is " << (int)sizeMB << " MB." << std::endl;
      std::cerr << "Loading large files may be slow. Continue? [y/N] ";
      std::string response;
      std::getline(std::cin, response);
      if (response.empty() || (response[0] != 'y' && response[0] != 'Y')) {
        std::cerr << "Aborted." << std::endl;
        return 0;
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
