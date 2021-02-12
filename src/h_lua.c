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

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "common.h"
#include "h_error.h"
#include "h_lua.h"
#include "haserl.h"

extern lua_State *lua_vm;

int
h_lua_loadfile(lua_State *L)
{
	const char *filename = luaL_checkstring(L, 1);

	if (luaL_loadfile(L, filename)) {
		die_with_message("Cannot load file '%s': %s", filename,
		                 lua_tostring(L, -1));
	}

	return 1;
}

void
lua_doscript(char *name)
{
	int status = luaL_loadfile(lua_vm, name) || lua_pcall(lua_vm, 0, LUA_MULTRET, 0);

	if (status && !lua_isnil(lua_vm, -1)) {
		const char *msg = lua_tostring(lua_vm, -1);
		if (msg == NULL) {
			msg = "(error object is not a string)";
		}
		die_with_message("%s", msg);
	}
}
