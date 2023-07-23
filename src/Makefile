PACKAGE ?= haserl
VERSION ?= 0.9.35
CFLAGS += -fpic -Wall -Werror
CPPFLAGS += -D_GNU_SOURCE -DVERSION=\"$(VERSION)\" -DPACKAGE=\"$(PACKAGE)\"
DESTDIR ?= /usr/local

WITH_LUA ?= luajit
LUA_CFLAGS := $(shell pkg-config --cflags -- $(WITH_LUA))
LUA_LDFLAGS := $(shell pkg-config --libs -- $(WITH_LUA))

haserl: haserl.o main.o common.o buffer.o sliding_buffer.o multipart.o h_lua.o
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LUA_LDFLAGS) -o $@ $^

haserl.o: haserl.c haserl.h common.h buffer.h sliding_buffer.h multipart.h
main.o: main.c haserl.h common.h h_lua.h

common.o: common.c common.h haserl.h
buffer.o: buffer.c buffer.h common.h
sliding_buffer.o: sliding_buffer.c sliding_buffer.h haserl.h common.h
multipart.o: multipart.c multipart.h haserl.h common.h buffer.h sliding_buffer.h
h_lua.o: h_lua.c h_lua.h haserl.h common.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LUA_CFLAGS) -c -o $@ $<

.PHONY: install
install: haserl haserl.1
	install -Dm755 haserl $(DESTDIR)/bin/haserl
	install -Dm644 haserl.1 $(DESTDIR)/share/man/man1/haserl.1
