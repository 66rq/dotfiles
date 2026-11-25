CC = cc
CFLAGS = -O2 -Wall -Wextra -std=c99
PREFIX = /usr/local
BINDIR = $(DESTDIR)$(PREFIX)/bin
XSESSIONSDIR = $(DESTDIR)/usr/share/xsessions
LDLIBS = -lX11 -lXrender -lXcomposite -lXdamage -lXfixes -lXi -lXpresent -lXrandr -lm
MODULES = $(wildcard modules/*.h)

.PHONY: all clean install uninstall reload

all: havel

havel: havel.c config.h $(MODULES)
	$(CC) $(CFLAGS) -o $@ havel.c $(LDLIBS)

clean:
	rm -f havel

install: havel
	install -d $(BINDIR)
	install -m755 havel $(BINDIR)
	install -d $(XSESSIONSDIR)
	install -m644 havel.desktop $(XSESSIONSDIR)/havel.desktop

reload: install
	@pkill -USR1 -x havel || true

uninstall:
	rm -f $(BINDIR)/havel
	rm -f $(XSESSIONSDIR)/havel.desktop
