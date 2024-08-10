#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "buffer.h"
#include "sliding_buffer.h"
#include "multipart.h"

#include "haserl.h"

/* Convert 2 char hex string into char it represents
 * (from http://www.jmarshall.com/easy/cgi)
 * (specifically http://www.jmarshall.com/easy/cgi/getcgi.c.txt) */
char
x2c(const char *what)
{
	char digit;

	digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A') + 10 : (what[0] - '0'));
	digit *= 16;
	digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A') + 10 : (what[1] - '0'));
	return digit;
}

/* unsescape %xx to the characters they represent */
/* Modified by Juris Feb 2007 */
void
unescape_url(char *url)
{
	int i, j;

	for (i = 0, j = 0; url[j]; ++i, ++j) {
		if ((url[i] = url[j]) != '%') {
			continue;
		}
		if (!url[j + 1] || !url[j + 2]) {
			break;
		}
		url[i] = x2c(url + j + 1);
		j += 2;
	}
	url[i] = 0;
}

/* if HTTP_COOKIE is passed as an environment variable,
 * attempt to parse its values into a global variable */
void
ReadCookie(void)
{
	char *token;
	char *cookie;

	if (!(cookie = getenv("HTTP_COOKIE"))) {
		return;
	}
	cookie = xstrdup(cookie);

	/* split on ; to extract name value pairs */
	token = strtok(cookie, ";");
	while (token) {
		/* skip leading spaces */
		while (*token == ' ') {
			token++;
		}
		list_add(&global.cookie, token);
		token = strtok(NULL, ";");
	}
	free(cookie);
}

/* Read CGI variables from query string, and put it into a global variable */
void
ReadQuery(void)
{
	char *token;
	char *query;
	int i;

	if (!(query = getenv("QUERY_STRING"))) {
		return;
	}
	query = xstrdup(query);

	/* change pluses into spaces */
	for (i = 0; query[i]; i++) {
		if (query[i] == '+') {
			query[i] = ' ';
		}
	}

	/* split on & and ; to extract name value pairs */
	token = strtok(query, "&;");
	while (token) {
		unescape_url(token);
		list_add(&global.get, token);
		token = strtok(NULL, "&;");
	}
	free(query);
}

/* Read CGI variables from stdin (for POST queries) */
void
ReadForm(void)
{
	size_t length = 0;
	size_t max_len;
	int urldecoding = 0;
	char *matchstr = "";
	size_t i, j, x;
	sliding_buffer_t sbuf;
	buffer_t token;
	char *data;
	const char *CONTENT_LENGTH = "CONTENT_LENGTH";
	const char *CONTENT_TYPE = "CONTENT_TYPE";
	char *content_type;
	char *content_length;

	if (!(content_length = getenv(CONTENT_LENGTH))) {
		return;
	} else {
		s_buffer_init(&sbuf, 32768);
		sbuf.fh = 0;

		if (!(sbuf.maxread = strtoul(content_length, NULL, 10))) {
			return;
		}
	}

	if ((content_type = getenv(CONTENT_TYPE))) {
		if (!strncasecmp(content_type, "multipart/form-data", 19)) {
			/* This is a mime request, we need to go to the mime handler */
			multipart_handler();
			return;
		}

		/* at this point its either urlencoded or some other blob */
		if (!strncasecmp(content_type, "application/x-www-form-urlencoded", 33)) {
			urldecoding = 1;
			matchstr = "&";
		}
	}

	/* otherwise, assume just a binary octet stream
	 *
	 * These were set in the variable definition - just leave them alone
	 * urldecoding = 0;
	 * matchstr = ""; */

	max_len = global.uploadkb * 1024;

	buffer_init(&token);

	if (!urldecoding) {
		buffer_add(&token, "body=", 5);
	}

	do {
		/* x is true if this token ends with a matchstr or is at the end of stream */
		x = s_buffer_read(&sbuf, matchstr);
		length += sbuf.len;
		if (length > max_len) {
			die("Attempted to send content larger than allowed limits");
		}

		if (!x || token.data) {
			buffer_add(&token, sbuf.segment, sbuf.len);
		}

		if (x) {
			data = sbuf.segment;
			sbuf.segment[sbuf.len] = 0;
			if (token.data) {
				/* add the ASCIIZ */
				buffer_add(&token, sbuf.segment + sbuf.len, 1);
				data = token.data;
			}

			if (urldecoding) {
				/* change pluses into spaces */
				j = strlen(data);
				for (i = 0; i <= j; i++) {
					if (data[i] == '+') {
						data[i] = ' ';
					}
				}
				unescape_url(data);
			}
			list_add(&global.post, data);
			if (token.data) {
				buffer_reset(&token);
			}
		}
	} while (!sbuf.eof);
	s_buffer_destroy(&sbuf);
	buffer_destroy(&token);
}

void
haserl(void)
{
	/* Read the request data */
	char *request_method;
	ReadCookie();
	if ((request_method = getenv("REQUEST_METHOD"))) {
		if (!strcasecmp(request_method, "GET") ||
		    !strcasecmp(request_method, "DELETE")) {
			ReadQuery();
		}

		if (!strcasecmp(request_method, "POST") ||
		    !strcasecmp(request_method, "PUT")) {
			ReadForm();
		}
	}
}
