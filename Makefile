
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
HELPERDIR ?= $(PREFIX)/lib/makekeeper
MANDIR ?= $(PREFIX)/share/man/man1
FISHDIR ?= /usr/share/fish/vendor_completions.d
DESKTOPDIR ?= $(PREFIX)/share/applications
ICONDIR ?= $(PREFIX)/share/icons/hicolor/128x128/apps
ICONSVGDIR ?= $(PREFIX)/share/icons/hicolor/scalable/apps
# polkit only reads policies from /usr/share, whatever the prefix
POLKITDIR ?= /usr/share/polkit-1/actions
SED ?= sed
INSTALL ?= install
RM ?= rm

BINARY = build/makekeeper
HELPER = build/makekeeper-helper
PROGRAM = makekeeper
MANPAGE = makekeeper.1
FISHCOMP = makekeeper.fish
DESKTOPFILE = makekeeper.desktop
ICONFILE = makekeeper-128.png
ICONSVGFILE = makekeeper.svg
POLICYIN = makekeeper.policy.in
POLICYFILE = io.github.creativecodecat.makekeeper.policy
DESTDIR ?=

.PHONY: all build install uninstall clean

all: build

build:
	mkdir -p build && cd build && cmake .. && make

install: build
	$(INSTALL) -d "$(DESTDIR)$(BINDIR)"
	$(INSTALL) -d "$(DESTDIR)$(HELPERDIR)"
	$(INSTALL) -d "$(DESTDIR)$(POLKITDIR)"
	$(INSTALL) -d "$(DESTDIR)$(MANDIR)"
	$(INSTALL) -d "$(DESTDIR)$(FISHDIR)"
	$(INSTALL) -d "$(DESTDIR)$(DESKTOPDIR)"
	$(INSTALL) -d "$(DESTDIR)$(ICONDIR)"
	$(INSTALL) -d "$(DESTDIR)$(ICONSVGDIR)"
	$(INSTALL) -m 755 "$(BINARY)" "$(DESTDIR)$(BINDIR)/$(PROGRAM)"
	$(INSTALL) -m 755 "$(HELPER)" "$(DESTDIR)$(HELPERDIR)/makekeeper-helper"
	$(SED) "s|@HELPER_PATH@|$(HELPERDIR)/makekeeper-helper|" "$(POLICYIN)" > build/$(POLICYFILE)
	$(INSTALL) -m 644 build/$(POLICYFILE) "$(DESTDIR)$(POLKITDIR)/$(POLICYFILE)"
	$(INSTALL) -m 644 "$(MANPAGE)" "$(DESTDIR)$(MANDIR)/$(MANPAGE)"
	$(INSTALL) -m 644 "$(FISHCOMP)" "$(DESTDIR)$(FISHDIR)/$(FISHCOMP)"
	$(INSTALL) -m 644 "$(DESKTOPFILE)" "$(DESTDIR)$(DESKTOPDIR)/$(DESKTOPFILE)"
	$(INSTALL) -m 644 "$(ICONFILE)" "$(DESTDIR)$(ICONDIR)/$(ICONFILE)"
	$(INSTALL) -m 644 "$(ICONSVGFILE)" "$(DESTDIR)$(ICONSVGDIR)/$(ICONSVGFILE)"

uninstall:
	$(RM) -f "$(DESTDIR)$(BINDIR)/$(PROGRAM)"
	$(RM) -rf "$(DESTDIR)$(HELPERDIR)"
	$(RM) -f "$(DESTDIR)$(POLKITDIR)/$(POLICYFILE)"
	$(RM) -f "$(DESTDIR)$(MANDIR)/$(MANPAGE)"
	$(RM) -f "$(DESTDIR)$(FISHDIR)/$(FISHCOMP)"
	$(RM) -f "$(DESTDIR)$(DESKTOPDIR)/$(DESKTOPFILE)"
	$(RM) -f "$(DESTDIR)$(ICONDIR)/$(ICONFILE)"
	$(RM) -f "$(DESTDIR)$(ICONSVGDIR)/$(ICONSVGFILE)"
	$(RM) -rf /var/lib/makekeeper

clean:
	rm -rf build


