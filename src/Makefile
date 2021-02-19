PACKAGE ?= haserl
VERSION ?= 0.9.35
CPPFLAGS ?= -D_GNU_SOURCE -DVERSION=\"$(VERSION)\" -DPACKAGE=\"$(PACKAGE)\"
DESTDIR ?= /usr/local

WITH_LUA ?= luajit
CFLAGS += $(shell pkg-config --cflags -- $(WITH_LUA))
LIBS += $(shell pkg-config --libs -- $(WITH_LUA))

SOURCES = main.c main.h common.c common.h \
          sliding_buffer.c sliding_buffer.h \
          h_error.c h_error.h rfc2388.c rfc2388.h \
          h_lua.c h_lua.h haserl.c haserl.h
haserl: $(SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) \
		-o $@ $(filter %.c,$^) $(LIBS)

install: haserl haserl.1
	install -Dm755 haserl $(DESTDIR)/bin/haserl
	install -Dm755 haserl.1 $(DESTDIR)/share/man/man1/haserl.1
