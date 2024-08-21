PACKAGE ?= haserl
VERSION ?= 0.10.0
URL ?= http://github.com/neeshy/haserl
CFLAGS += -fPIE -Wall -Werror
CPPFLAGS += -D_GNU_SOURCE -DPACKAGE=\"$(PACKAGE)\" -DVERSION=\"$(VERSION)\" -DURL=\"$(URL)\"
DESTDIR ?= /usr/local

WITH_LUA ?= luajit
LUA_CFLAGS := $(shell pkg-config --cflags -- $(WITH_LUA))
LUA_LDFLAGS := $(shell pkg-config --libs -- $(WITH_LUA))

haserl: haserl.o main.o common.o buffer.o sliding_buffer.o multipart.o h_lua.o
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LUA_LDFLAGS) -o $@ $^

haserl.o: haserl.c haserl.h common.h buffer.h multipart.h
main.o: main.c haserl.h common.h buffer.h h_lua.h
common.o: common.c common.h buffer.h
buffer.o: buffer.c buffer.h common.h
sliding_buffer.o: sliding_buffer.c sliding_buffer.h buffer.h common.h
multipart.o: multipart.c multipart.h common.h buffer.h sliding_buffer.h

h_lua.o: h_lua.c h_lua.h common.h buffer.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LUA_CFLAGS) -c -o $@ $<

.PHONY: install
install: haserl haserl.1
	install -Dm755 haserl $(DESTDIR)/bin/haserl
	install -Dm644 haserl.1 $(DESTDIR)/share/man/man1/haserl.1
