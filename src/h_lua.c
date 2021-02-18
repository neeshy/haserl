/* ---------------------------------------------------------------------------
 * Copyright 2003-2011 (inclusive) Nathan Angelacos
 *                   (nangel@users.sourceforge.net)
 *
 *   This file is part of haserl.
 *
 *   Haserl is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   as published by the Free Software Foundation.
 *
 *   Haserl is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with haserl.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ------------------------------------------------------------------------ */

#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "common.h"
#include "h_lua.h"
#include "h_error.h"

lua_State *lua_vm = NULL;

void
lua_putenv(const list_t *env, const char *tbl)
{
	char *str, *value;

	lua_getglobal(lua_vm, tbl);
	if (lua_isnil(lua_vm, -1)) {
		lua_pop(lua_vm, 1);
		lua_newtable(lua_vm);
		lua_setglobal(lua_vm, tbl);
		lua_getglobal(lua_vm, tbl);
	}

	while (env) {
		str = env->buf;
		value = memchr(str, '=', strlen(str));
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

int
lua_loadfile(lua_State *L)
{
	const char *filename = luaL_checkstring(L, 1);

	if (luaL_loadfile(L, filename)) {
		die("Cannot load file '%s': %s", filename, lua_tostring(L, -1));
	}

	return 1;
}

void
lua_doscript(const char *name)
{
	if (luaL_loadfile(lua_vm, name) || lua_pcall(lua_vm, 0, LUA_MULTRET, 0)) {
		die("%s", lua_tostring(lua_vm, -1));
	}
}

void
lua_setup(void)
{
	/* create a lua instance */
	lua_vm = luaL_newstate();
	luaL_openlibs(lua_vm);

	/* and put the vars in the vm */
	lua_putenv(global.get, "GET");
	lua_putenv(global.post, "POST");
	lua_putenv(global.cookie, "COOKIE");
	lua_putenv(global.haserl, "HASERL");

	/* register our open function in the haserl table */
	lua_pushcfunction(lua_vm, lua_loadfile);
	lua_setglobal(lua_vm, "loadfile");
}

void
lua_destroy(void)
{
	/* close the lua instance */
	lua_close(lua_vm);
}
