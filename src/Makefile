PACKAGE ?= haserl
VERSION ?= 0.9.35
CPPFLAGS ?= -D_GNU_SOURCE -DVERSION=\"$(VERSION)\" -DPACKAGE=\"$(PACKAGE)\"
DESTDIR ?= /usr/local

LUA_SOURCE = h_lua_common.c h_lua_common.h
ifndef INCLUDE_LUASHELL
ifndef INCLUDE_LUACSHELL
INCLUDE_LUASHELL = yes
endif
endif
ifdef INCLUDE_LUASHELL
LUA_SOURCE += h_lua.c h_lua.h
DEFAULT_SHELL ?= lua
CPPFLAGS += -DINCLUDE_LUASHELL
endif
ifdef INCLUDE_LUACSHELL
LUA_SOURCE += h_luac.c h_luac.h
DEFAULT_SHELL ?= luac
CPPFLAGS += -DINCLUDE_LUACSHELL
endif
DEFAULT_SHELL ?= lua

WITH_LUA ?= lua
CFLAGS += $(shell pkg-config --cflags -- $(WITH_LUA))
LIBS += $(shell pkg-config --libs -- $(WITH_LUA))

SOURCES = common.c common.h sliding_buffer.c sliding_buffer.h \
          h_error.c h_error.h rfc2388.c rfc2388.h \
          $(LUA_SOURCE) haserl.c haserl.h haserl_lualib.inc
haserl: $(SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) \
		-o $@ $(filter %.c,$^) $(LIBS)
lua2c_LDFLAGS ?= -Wl,-E
lua2c: lua2c.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(lua2c_LDFLAGS) \
		-o $@ $^ $(LIBS)

haserl_lualib.inc: haserl_lualib.lua lua2c
	if ! ./lua2c haserl_lualib haserl_lualib.lua >$@; then \
		rm $@; \
		exit 1 ;\
	fi

install: haserl haserl.1
	install -Dm755 haserl $(DESTDIR)/bin/haserl
	install -Dm755 haserl.1 $(DESTDIR)/share/man/man1/haserl.1
