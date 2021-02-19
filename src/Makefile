PACKAGE ?= haserl
VERSION ?= 0.9.35
CFLAGS += -fpic -Wall -Werror
CPPFLAGS += -D_GNU_SOURCE -DVERSION=\"$(VERSION)\" -DPACKAGE=\"$(PACKAGE)\"
DESTDIR ?= /usr/local

WITH_LUA ?= luajit
LUA_CFLAGS := $(shell pkg-config --cflags -- $(WITH_LUA))
LUA_LDFLAGS := $(shell pkg-config --libs -- $(WITH_LUA))

#haserl: main.o h_lua.o libhaserl.so
haserl: haserl.o libhaserl.o main.o common.o h_error.o sliding_buffer.o rfc2388.o h_lua.o
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LUA_LDFLAGS) -o $@ $^

libhaserl.so: haserl.o libhaserl.o common.o h_error.o sliding_buffer.o rfc2388.o
	$(CC) $(CFLAGS) $(CPPFLAGS) -shared -o $@ $^

haserl.o: haserl.c libhaserl.h
libhaserl.o: libhaserl.c libhaserl.h haserl.h common.h h_error.h sliding_buffer.h rfc2388.h
main.o: main.c main.h haserl.h common.h h_error.h h_lua.h

common.o: common.c common.h haserl.h h_error.h
h_error.o: h_error.c h_error.h haserl.h
sliding_buffer.o: sliding_buffer.c sliding_buffer.h haserl.h common.h
rfc2388.o: rfc2388.c rfc2388.h haserl.h common.h h_error.h sliding_buffer.h
h_lua.o: h_lua.c h_lua.h haserl.h common.h h_error.h
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LUA_CFLAGS) -c -o $@ $<

.PHONY: install
install: haserl libhaserl.so haserl.1
	install -Dm755 haserl $(DESTDIR)/bin/haserl
	install -Dm755 libhaserl.so $(DESTDIR)/lib/libhaserl.so.$(VERSION)
	ln -sf libhaserl.so.$(VERSION) $(DESTDIR)/lib/libhaserl.so.$(firstword $(subst ., ,$(VERSION)))
	ln -sf libhaserl.so.$(VERSION) $(DESTDIR)/lib/libhaserl.so
	install -Dm644 haserl.1 $(DESTDIR)/share/man/man1/haserl.1
