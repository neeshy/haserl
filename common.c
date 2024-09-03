#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <lua.h>

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
drain(int fd)
{
	char c[2048];
	while (read(fd, c, sizeof(c)) > 0);
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
