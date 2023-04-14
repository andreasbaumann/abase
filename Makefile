include config.mk

.SUFFIXES:
.SUFFIXES: .o .c

HDR =\
	arg.h\
	util.h

LIBUTIL = libutil.a
LIBUTILSRC =\
	libutil/eprintf.c

LIB = $(LIBUTF) $(LIBUTIL)

BIN =\
	more

LIBUTILOBJ = $(LIBUTILSRC:.c=.o)
OBJ = $(BIN:=.o) $(LIBUTFOBJ) $(LIBUTILOBJ)
SRC = $(BIN:=.c)
MAN = $(BIN:=.1)

all: $(BIN)

$(BIN): $(LIB) $(@:=.o)

$(OBJ): $(HDR) config.mk

.o:
	$(CC) $(LDFLAGS) -o $@ $< $(LIB)

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

$(LIBUTIL): $(LIBUTILOBJ)
	$(AR) rc $@ $?
	$(RANLIB) $@

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f $(BIN) $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	for m in $(MAN); do sed "s/^\.Os abase/.Os abase $(VERSION)/g" < "$$m" > $(DESTDIR)$(MANPREFIX)/man1/"$$m"; done
	cd $(DESTDIR)$(MANPREFIX)/man1 && chmod 644 $(MAN)

uninstall:
	cd $(DESTDIR)$(PREFIX)/bin && rm -f $(BIN)
	cd $(DESTDIR)$(MANPREFIX)/man1 && rm -f $(MAN)

dist: clean
	mkdir -p abase-$(VERSION)
	cp -r LICENSE Makefile README TODO config.mk $(SRC) $(MAN) libutil $(HDR) abase-$(VERSION)
	tar -cf abase-$(VERSION).tar abase-$(VERSION)
	gzip abase-$(VERSION).tar
	rm -rf abase-$(VERSION)

abase-box: $(LIB) $(SRC)
	mkdir -p build
	cp $(HDR) build
	for f in $(SRC); do sed "s/^main(/$$(echo "$${f%.c}" | sed s/-/_/g)_&/" < $$f > build/$$f; done
	echo '#include <libgen.h>'                                                                                                     > build/$@.c
	echo '#include <stdio.h>'                                                                                                     >> build/$@.c
	echo '#include <stdlib.h>'                                                                                                    >> build/$@.c
	echo '#include <string.h>'                                                                                                    >> build/$@.c
	echo '#include "util.h"'                                                                                                      >> build/$@.c
	for f in $(SRC); do echo "int $$(echo "$${f%.c}" | sed s/-/_/g)_main(int, char **);"; done                                    >> build/$@.c
	echo 'int main(int argc, char *argv[]) { char *s = basename(argv[0]);'                                                        >> build/$@.c
	echo 'if(!strcmp(s,"abase-box")) { argc--; argv++; s = basename(argv[0]); } if(0) ;'                                          >> build/$@.c
	for f in $(SRC); do echo "else if(!strcmp(s, \"$${f%.c}\")) return $$(echo "$${f%.c}" | sed s/-/_/g)_main(argc, argv);"; done >> build/$@.c
	echo 'else {'                                                                                                                 >> build/$@.c
	for f in $(SRC); do echo "fputs(\"$${f%.c} \", stdout);"; done                                                                >> build/$@.c
	echo 'putchar(0xa); }; return 0; }'                                                                                           >> build/$@.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ build/*.c $(LIB)
	rm -r build

abase-box-install: abase-box
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f abase-box $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/abase-box
	for f in $(BIN); do ln -sf abase-box $(DESTDIR)$(PREFIX)/bin/"$$f"; done
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	for m in $(MAN); do sed "s/^\.Os abase/.Os abase $(VERSION)/g" < "$$m" > $(DESTDIR)$(MANPREFIX)/man1/"$$m"; done
	cd $(DESTDIR)$(MANPREFIX)/man1 && chmod 644 $(MAN)

abase-box-uninstall: uninstall
	cd $(DESTDIR)$(PREFIX)/bin && rm -f abase-box

clean:
	rm -f $(BIN) $(OBJ) $(LIB) abase-box abase-$(VERSION).tar.gz

.gitignore:
	{ printf '*.o\n' ; printf '/%s\n' $(LIB) $(BIN) ; } > $@

.PHONY: all install uninstall dist abase-box-install abase-box-uninstall clean .gitignore
