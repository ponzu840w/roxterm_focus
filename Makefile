PREFIX     ?= /usr/local
BINDIR     := $(PREFIX)/bin
CC         ?= gcc
CFLAGS     := -Wall -O2
LIBS       := -lX11
TARGET     := roxterm_focus

.PHONY: all install uninstall clean

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

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
