PREFIX       ?= /usr/local
BINDIR       := $(PREFIX)/bin
CC           ?= gcc
CFLAGS       := -Wall -O2
CFLAGS_DEBUG := -Wall -g -DDEBUG
LIBS         := -lX11
TARGET       := roxterm_focus
TARGET_DEBUG := $(TARGET)_debug

.PHONY: all install uninstall clean debug

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

debug: $(TARGET_DEBUG)

$(TARGET_DEBUG): $(TARGET).c
	$(CC) $(CFLAGS_DEBUG) -o $@ $< $(LIBS)

install: all
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/

	@echo
	@echo "================ Installation Complete ================"
	@echo "=> $(BINDIR)/$(TARGET) has been installed."
	@echo
	@echo "Next steps: set up auto-start of roxterm_focus in your X session."
	@echo " - Append the following line to ~/.desktop-session/startup:"
	@echo
	@echo "$(BINDIR)/$(TARGET) &"
	@echo
	@echo "========================================================"
	@echo

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	rm -f $(TARGET)
