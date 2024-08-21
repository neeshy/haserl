#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "buffer.h"
#include "common.h"
#include "multipart.h"

#include "haserl.h"

static int
unescape(char *where, const char *what)
{
#define decode() \
	if (c >= '0' && c <= '9') { \
		w |= c - '0'; \
	} else if (c >= 'A' && c <= 'F') { \
		w |= c - 'A' + 10; \
	} else if (c >= 'a' && c <= 'f') { \
		w |= c - 'a' + 10; \
	} else { \
		return 0; \
	}

	char c = what[0];
	char w = 0;
	decode()

	c = what[1];
	w <<= 4;
	decode()

	*where = w;

	return 1;
#undef decode
}

/* decode %xx to the bytes they represent */
static void
unescape_url(char *url)
{
	int i = 0;
	while (url[i]) {
		if ((*url = url[i]) == '%' && unescape(url, url + i + 1)) {
			i += 2;
		}
		url++;
	}
	*url = 0;
}

/* add a pair of strings delimited on '=' to a buffer */
static void
buffer_add_pair(buffer_t *buf, char *str)
{
	char *value = strchr(str, '=');
	if (value) {
		*value = 0;
		unescape_url(str);
		buffer_add(buf, str, value - str + 1);
		*value++ = '=';
		unescape_url(value);
		buffer_add(buf, value, strlen(value) + 1);
	} else {
		unescape_url(str);
		buffer_add(buf, str, strlen(str) + 1);
		buffer_add(buf, "", 1);
	}
}

static void
read_cookie(char *cookie, buffer_t *buf)
{
	/* split on ; to extract name value pairs */
	char *token = strtok(cookie, ";");
	while (token) {
		/* skip leading spaces */
		while (*token == ' ') token++;
		buffer_add_pair(buf, token);
		token = strtok(NULL, ";");
	}
}

static void
read_query(char *query, buffer_t *buf)
{
	/* change pluses into spaces */
	for (char *s = query; *s; s++) {
		if (*s == '+') *s = ' ';
	}

	/* split on & to extract name value pairs */
	char *token = strtok(query, "&");
	while (token) {
		buffer_add_pair(buf, token);
		token = strtok(NULL, "&");
	}
}

/* read CGI variables from stdin (for POST queries) */
static void
read_form(void)
{
	/* if upload_limit is zero, we die right here */
	if (!global.upload_max) {
		die("File uploads are not allowed");
	}

	size_t max_len;
	char *content_length = getenv("CONTENT_LENGTH");
	if (!content_length) {
		die("Content length not specified");
	} else if (!(max_len = strtoul(content_length, NULL, 10))) {
		die("Invalid content length");
	} else if (max_len > global.upload_max) {
		die("Content length larger than allowed limits");
	}

	char *content_type = getenv("CONTENT_TYPE");
	if (content_type && !strncasecmp(content_type, "multipart/form-data", 19)) {
		multipart_handler();
		return;
	}

	buffer_t buf;
	buffer_alloc(&buf, CHUNK_SIZE);

	ssize_t n = read(0, buf.ptr, CHUNK_SIZE);
	while (n > 0) {
		buf.ptr += n;

		/* maximum size for non-multipart/form-data requests is CHUNK_SIZE */
		if (buf.ptr - buf.data >= CHUNK_SIZE) {
			buffer_destroy(&buf);
			die("Reached maximum allowed input length");
		}

		n = read(0, buf.ptr, buf.limit - buf.ptr);
	}

	if (n == -1) {
		buffer_destroy(&buf);
		die_status(errno, "read: %s", strerror(errno));
	}

	/* add the ASCIIZ */
	buffer_add(&buf, "", 1);
	if (content_type && !strncasecmp(content_type, "application/x-www-form-urlencoded", 33)) {
		read_query(buf.data, &global.post);
	} else {
		/* treat input as an opaque octet stream */
		buffer_add(&global.post, "body", 5);
		buffer_add(&global.post, buf.data, buf.ptr - buf.data);
	}
	buffer_destroy(&buf);

}

/* read the request data */
void
haserl(void)
{
	char *cookie = getenv("HTTP_COOKIE");
	if (cookie) {
		cookie = xstrdup(cookie);
		read_cookie(cookie, &global.cookie);
		free(cookie);
	}

	char *request_method = getenv("REQUEST_METHOD");
	if (request_method) {
		if (!strcasecmp(request_method, "GET") ||
		    !strcasecmp(request_method, "DELETE")) {
			char *query = getenv("QUERY_STRING");
			if (query) {
				query = xstrdup(query);
				read_query(query, &global.get);
				free(query);
			}
		} else if (!strcasecmp(request_method, "POST") ||
		           !strcasecmp(request_method, "PUT")) {
			read_form();
		}
	}
}
