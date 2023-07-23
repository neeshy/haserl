#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "common.h"
#include "h_error.h"

#include "h_lua.h"

void
lua_putenv(lua_State *lua_vm, const list_t *env, const char *tbl)
{
	char *str, *value;

	lua_getglobal(lua_vm, tbl);
	if (!lua_istable(lua_vm, -1)) {
		lua_pop(lua_vm, 1);
		lua_newtable(lua_vm);
		lua_setglobal(lua_vm, tbl);
		lua_getglobal(lua_vm, tbl);
	}

	while (env) {
		str = env->buf;
		value = strchr(str, '=');
		if (value) {
			*value = '\0';
			value++;
		} else {
			value = str + strlen(str);
		}

		lua_pushstring(lua_vm, value);
		lua_setfield(lua_vm, -2, str);

		env = env->next;
	}

	lua_pop(lua_vm, 1);
}

void
lua_doscript(const char *name)
{
	/* create a lua instance */
	lua_State *lua_vm = luaL_newstate();
	luaL_openlibs(lua_vm);

	/* and put the vars in the vm */
	lua_putenv(lua_vm, global.get, "GET");
	lua_putenv(lua_vm, global.post, "POST");
	lua_putenv(lua_vm, global.form, "FORM");
	lua_putenv(lua_vm, global.cookie, "COOKIE");

	if (luaL_loadfile(lua_vm, name) || lua_pcall(lua_vm, 0, LUA_MULTRET, 0)) {
		die("%s", lua_tostring(lua_vm, -1));
	}

	/* close the lua instance */
	lua_close(lua_vm);
}
