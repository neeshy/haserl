#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "common.h"

/* assign default values to the global structure */
haserl_t global = {
	.upload_max = 0,           /* maximum upload size (0 disables file uploads) */
	.upload_dir = "/tmp",      /* where to upload to */
	.L = NULL,
};

/* allocate memory or die, busybox style. */
void *
xmalloc(size_t size)
{
	void *buf = malloc(size);
	if (!buf) {
		die_status(errno, "malloc: %s", strerror(errno));
	}
	memset(buf, 0, size);
	return buf;
}

/* realloc or die */
void *
xrealloc(void *buf, size_t size)
{
	if (!(buf = realloc(buf, size)) && size) {
		die_status(errno, "realloc: %s", strerror(errno));
	}
	return buf;
}

/* strdup or die */
char *
xstrdup(const char *s)
{
	char *ret = strdup(s);
	if (!ret) {
		die_status(errno, "strdup: %s", strerror(errno));
	}
	return ret;
}

void
lua_init(void)
{
	lua_State *L = luaL_newstate();
	global.L = L;
	luaL_openlibs(L);

	lua_newtable(L);
	lua_setglobal(L, "GET");

	lua_newtable(L);
	lua_setglobal(L, "POST");

	lua_newtable(L);
	lua_setglobal(L, "FORM");

	lua_newtable(L);
	lua_setglobal(L, "COOKIE");
}

void
lua_set(const char *tbl, const char *key, size_t key_size, const char *value, size_t value_size)
{
	lua_State *L = global.L;

	lua_getglobal(L, tbl);
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		lua_newtable(L);
		lua_pushvalue(L, -1);
		lua_setglobal(L, tbl);
	}

	lua_pushlstring(L, key, key_size);
	lua_pushlstring(L, value, value_size);
	lua_settable(L, -3);
	lua_pop(L, 1);
}

static int
lua_print(lua_State *L)
{
	int n = lua_gettop(L);

	lua_getglobal(L, "BUFFER");
	lua_pushinteger(L, lua_objlen(L, -1) + 1);
	lua_insert(L, 1);
	lua_insert(L, 1);

	lua_getglobal(L, "string");
	lua_getfield(L, -1, "format");
	lua_remove(L, -2);
	lua_insert(L, -n - 1);
	lua_call(L, n, 1);

	lua_settable(L, -3);

	return 0;
}

void
lua_exec(const char *filename)
{
	lua_State *L = global.L;

	lua_newtable(L);
	lua_setglobal(L, "BUFFER");

	lua_pushcfunction(L, lua_print);
	lua_setglobal(L, "print");

	if (luaL_loadfile(L, filename) || lua_pcall(L, 0, 0, 0)) {
		die("%s", lua_tostring(L, -1));
	}

	lua_getglobal(L, "table");
	lua_getfield(L, -1, "concat");
	lua_remove(L, -2);
	lua_getglobal(L, "BUFFER");
	if (lua_pcall(L, 1, 1, 0)) {
		die("%s", lua_tostring(L, -1));
	}

	size_t len;
	const char *buffer = lua_tolstring(L, -1, &len);
	if (!buffer) {
		die("lua_tolstring: BUFFER not accessible");
	}

	ssize_t n = write(1, buffer, len);
	lua_pop(L, 1);
	if (n != len || n == -1) {
		die_status(errno, "write: %s", strerror(errno));
	}
}

void
drain(int fd)
{
	char c[2048];
	while (read(fd, c, sizeof(c)));
}

/* print an error message and terminate.
 * if there's a request method, HTTP headers are added. */
static void
vdie(int status, const char *s, va_list ap)
{
	if (getenv("REQUEST_METHOD")) {
		dprintf(1, "HTTP/1.0 500 Server Error\r\n"
		        "Content-Type: text/html\r\n\r\n"
		        "<html><body><b><font color='#C00'>" PACKAGE
		        " CGI Error</font></b><br><pre>\r\n");
		vdprintf(1, s, ap);
		dprintf(1, "\r\n</pre></body></html>");
	} else {
		vdprintf(2, s, ap);
		dprintf(2, "\n");
	}

	drain(0);
	lua_close(global.L);

	exit(status);
}

void
die_status(int status, const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	vdie(status, s, ap);
	va_end(ap);
}

void
die(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	vdie(-1, s, ap);
	va_end(ap);
}
