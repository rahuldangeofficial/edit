CC = gcc
CFLAGS = -O3 -Wall -Wextra -pedantic -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=700
LIBS = -lncurses
BIN = edit
SRC = edit.c
PREFIX = /usr/local

.PHONY: all clean install uninstall debug

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(BIN)

install: $(BIN)
	@echo "🔧 Installing $(BIN) to $(PREFIX)/bin..."
	sudo install -m 755 $(BIN) $(PREFIX)/bin/$(BIN)
	@echo "Installed. You can now use 'edit' globally."

uninstall:
	@echo "🗑️ Uninstalling $(BIN)..."
	sudo rm -f $(PREFIX)/bin/$(BIN)
	@echo "Uninstalled."

clean:
	rm -f $(BIN)

debug:
	$(CC) -g -Wall -DDEBUG $(SRC) $(LIBS) -o $(BIN)