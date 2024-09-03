PACKAGE ?= haserl
VERSION ?= 0.10.0
URL ?= http://github.com/neeshy/haserl
CFLAGS += -fPIE -Wall -Werror
CPPFLAGS += -D_GNU_SOURCE -DPACKAGE=\"$(PACKAGE)\" -DVERSION=\"$(VERSION)\" -DURL=\"$(URL)\"
DESTDIR ?= /usr/local

WITH_LUA ?= luajit
LUA_CFLAGS := $(shell pkg-config --cflags -- $(WITH_LUA))
LUA_LDFLAGS := $(shell pkg-config --libs -- $(WITH_LUA))

haserl: haserl.o multipart.o main.o common.o lua.o buffer.o sliding_buffer.o
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LUA_LDFLAGS) -o $@ $^

haserl.o: haserl.c common.h util.h buffer.h
multipart.o: multipart.c common.h util.h buffer.h sliding_buffer.h
main.o: main.c common.h util.h
common.o: common.c common.h util.h
lua.o: lua.c common.h util.h
buffer.o: buffer.c buffer.h util.h
sliding_buffer.o: sliding_buffer.c sliding_buffer.h util.h

haserl.o multipart.o main.o common.o lua.o buffer.o sliding_buffer.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LUA_CFLAGS) -c -o $@ $<

.PHONY: install
install: haserl haserl.1
	install -Dm755 haserl $(DESTDIR)/bin/haserl
	install -Dm644 haserl.1 $(DESTDIR)/share/man/man1/haserl.1
