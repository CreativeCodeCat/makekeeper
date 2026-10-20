
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man/man1
CARGO ?= cargo
INSTALL ?= install
RM ?= rm

BINARY = target/release/makekeeper
PROGRAM = makekeeper
MANPAGE = makekeeper.1
DESTDIR ?=

.PHONY: all build check install uninstall clean

.PHONY: all build check install uninstall clean

all: build

build:
	$(CARGO) build --release

check:
	$(CARGO) check

install: build
	$(INSTALL) -d "$(DESTDIR)$(BINDIR)"
	$(INSTALL) -d "$(DESTDIR)$(MANDIR)"
	$(INSTALL) -m 755 "$(BINARY)" "$(DESTDIR)$(BINDIR)/$(PROGRAM)"
	$(INSTALL) -m 644 "$(MANPAGE)" "$(DESTDIR)$(MANDIR)/$(MANPAGE)"

uninstall:
	$(RM) -f "$(DESTDIR)$(BINDIR)/$(PROGRAM)"
	$(RM) -f "$(DESTDIR)$(MANDIR)/$(MANPAGE)"

clean:
	$(CARGO) clean
