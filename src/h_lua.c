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

/* this is not a mistake. We are including the
 * definition of the lualib here */
#include "haserl_lualib.inc"

lua_State *lua_vm = NULL;

void
lua_putenv(char *str)
{
	char *value;

	value = memchr(str, '=', strlen(str));
	if (value) {
		*value = '\0';
		value++;
	} else {
		value = str + strlen(str);
	}

	lua_getglobal(lua_vm, "haserl");
	lua_pushstring(lua_vm, "myputenv");
	lua_gettable(lua_vm, -2);
	lua_pushstring(lua_vm, str);
	lua_pushstring(lua_vm, value);
	lua_call(lua_vm, 2, 0);
	return;
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
lua_doscript(char *name)
{
	if (luaL_loadfile(lua_vm, name) || lua_pcall(lua_vm, 0, LUA_MULTRET, 0)) {
		die("%s", lua_tostring(lua_vm, -1));
	}
}

void
lua_setup(list_t *env)
{
	/* create a lua instance */
	lua_vm = luaL_newstate();
	luaL_openlibs(lua_vm);

	/* and load our haserl library */
	if (luaL_loadbuffer(lua_vm,
	                    (const char *)&haserl_lualib, sizeof(haserl_lualib),
	                    "luascript.lua") || lua_pcall(lua_vm, 0, 0, 0)) {
		die("Error passing the lua library to the lua vm: %s",
		    lua_tostring(lua_vm, -1));
	}

	/* and put the vars in the vm */
	while (env) {
		lua_putenv(env->buf);
		env = env->next;
	}

	/* register our open function in the haserl table */
	lua_getglobal(lua_vm, "haserl");
	lua_pushstring(lua_vm, "loadfile");
	lua_pushcfunction(lua_vm, lua_loadfile);
	lua_settable(lua_vm, -3);
}

void
lua_destroy(void)
{
	/* close the lua instance */
	lua_close(lua_vm);
}
