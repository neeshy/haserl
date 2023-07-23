#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "common.h"

/* Assign default values to the global structure */
haserl_t global = {
	.uploadkb = 2048,         /* how big an upload do we allow (0 for none) */
	.uploaddir = "/tmp",      /* where to upload to                         */
	.get = NULL,
	.post = NULL,
	.form = NULL,
	.cookie = NULL,
};

/* allocate memory or die, busybox style. */
void *
xmalloc(size_t size)
{
	void *buf;

	if (!(buf = malloc(size))) {
		die("Memory Allocation Failure");
	}
	memset(buf, 0, size);
	return buf;
}

/* realloc memory, or die xmalloc style. */
void *
xrealloc(void *buf, size_t size)
{
	if (!(buf = realloc(buf, size))) {
		die("Memory Re-allocation Failure");
	}
	return buf;
}

/* print an error message and terminate. If there's a request method, then HTTP
 * headers are added. */
void
die(const char *s, ...)
{
	va_list p;
	FILE *fo = stderr;

	if (getenv("REQUEST_METHOD")) {
		fo = stdout;
		fprintf(fo, "HTTP/1.0 500 Server Error\r\n"
		        "Content-Type: text/html\r\n\r\n"
		        "<html><body><b><font color='#C00'>" PACKAGE
		        " CGI Error</font></b><br><pre>\r\n");
	}

	va_start(p, s);
	vfprintf(fo, s, p);
	va_end(p);
	printf("\r\n");

	if (getenv("REQUEST_METHOD")) {
		fprintf(fo, "</pre></body></html>");
	}

	exit(-1);
}

/* adds or replaces the "key=value" value in the env_list chain */
void
list_add(list_t **env, const char *str)
{
	list_t *cur = *env;
	list_t *prev = NULL;
	char *entry = NULL;
	char *temp = NULL;
	size_t keylen;

	temp = strchr(str, '=');
	/* if we don't have an equal sign, exit early */
	if (!temp) {
		return;
	}

	keylen = (size_t)(temp - str) + 1;
	entry = strdup(str);
	/* does the value already exist? */
	while (cur) {
		/* if found a matching key */
		if (!strncmp(cur->buf, entry, keylen)) {
			/* delete the old entry */
			free(cur->buf);
			if (prev) {
				prev->next = cur->next;
				free(cur);
				cur = prev->next;
			} else {
				free(cur);
				cur = NULL;
			}
		} else {
			prev = cur;
			cur = cur->next;
		}
	}

	/* add the value to the end of the chain */
	cur = xmalloc(sizeof(list_t));
	cur->buf = entry;
	if (prev) {
		prev->next = cur;
	} else {
		*env = cur;
	}
}

/* free list_t */
void
list_destroy(list_t *list)
{
	list_t *next;

	while (list) {
		next = list->next;
		free(list->buf);
		free(list);
		list = next;
	}
}
