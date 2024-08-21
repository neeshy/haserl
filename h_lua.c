#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "buffer.h"
#include "common.h"

#include "h_lua.h"

static void
lua_putenv(lua_State *L, const buffer_t *buf, const char *tbl)
{
	lua_getglobal(L, tbl);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		lua_newtable(L);
		lua_setglobal(L, tbl);
		lua_getglobal(L, tbl);
	}

	char *key = buf->data;
	char *value;
	while (key && key < buf->ptr && (value = memchr(key, 0, buf->ptr - key))) {
		lua_pushstring(L, ++value);
		lua_setfield(L, -2, key);
		if ((key = memchr(value, 0, buf->ptr - value))) key++;
	}

	lua_pop(L, 1);
}

void
lua_doscript(const char *name)
{
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);

	lua_putenv(L, &global.get, "GET");
	lua_putenv(L, &global.post, "POST");
	lua_putenv(L, &global.form, "FORM");
	lua_putenv(L, &global.cookie, "COOKIE");

	if (luaL_loadfile(L, name) || lua_pcall(L, 0, 0, 0)) {
		die("%s", lua_tostring(L, -1));
	}

	lua_close(L);
}
