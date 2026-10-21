
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man/man1
FISHDIR ?= /usr/share/fish/vendor_completions.d
CARGO ?= cargo
INSTALL ?= install
RM ?= rm

BINARY = target/release/makekeeper
PROGRAM = makekeeper
MANPAGE = makekeeper.1
FISHCOMP = makekeeper.fish
DESTDIR ?=

.PHONY: all build check install uninstall clean

all: build

build:
	$(CARGO) build --release

check:
	$(CARGO) check

install: build
	$(INSTALL) -d "$(DESTDIR)$(BINDIR)"
	$(INSTALL) -d "$(DESTDIR)$(MANDIR)"
	$(INSTALL) -d "$(DESTDIR)$(FISHDIR)"
	$(INSTALL) -m 755 "$(BINARY)" "$(DESTDIR)$(BINDIR)/$(PROGRAM)"
	$(INSTALL) -m 644 "$(MANPAGE)" "$(DESTDIR)$(MANDIR)/$(MANPAGE)"
	$(INSTALL) -m 644 "$(FISHCOMP)" "$(DESTDIR)$(FISHDIR)/$(FISHCOMP)"

uninstall:
	$(RM) -f "$(DESTDIR)$(BINDIR)/$(PROGRAM)"
	$(RM) -f "$(DESTDIR)$(MANDIR)/$(MANPAGE)"
	$(RM) -f "$(DESTDIR)$(FISHDIR)/$(FISHCOMP)"

clean:
	$(CARGO) clean
