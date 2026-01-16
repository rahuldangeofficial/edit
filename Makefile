CXX = g++
# -D_POSIX_C_SOURCE=200809L: Ensures visibility of setenv, fsync, etc. on Linux
CXXFLAGS = -Wall -Wextra -Werror -pedantic -std=c++17 -O3 -D_XOPEN_SOURCE_EXTENDED -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lncurses

SRC_DIR = src
OBJ_DIR = obj
TARGET = edit
PREFIX = /usr/local

SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

install: $(TARGET)
	@echo "Installing $(TARGET) to $(PREFIX)/bin..."
	@mkdir -p $(PREFIX)/bin
	@cp $(TARGET) $(PREFIX)/bin/$(TARGET)
	@chmod 755 $(PREFIX)/bin/$(TARGET)
	@echo "Done."

uninstall:
	@echo "Removing $(TARGET) from $(PREFIX)/bin..."
	@rm -f $(PREFIX)/bin/$(TARGET)
	@echo "Done."

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean install uninstall