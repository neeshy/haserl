/* --------------------------------------------------------------------------
 * Copyright 2003-2015 (inclusive) Nathan Angelacos
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
 * -----
 * The x2c() and unescape_url() routines were taken from
 *  http://www.jmarshall.com/easy/cgi/getcgi.c.txt
 *
 * The comments in that text file state:
 *
 ***  Written in 1996 by James Marshall, james@jmarshall.com, except
 ***  that the x2c() and unescape_url() routines were lifted directly
 ***  from NCSA's sample program util.c, packaged with their HTTPD.
 ***     For the latest, see http://www.jmarshall.com/easy/cgi/
 * -----
 *
 * ------------------------------------------------------------------------- */

#include <stdlib.h>
#include <string.h>

#include "haserl.h"
#include "common.h"
#include "h_error.h"
#include "sliding_buffer.h"
#include "rfc2388.h"

#include "libhaserl.h"

/* Assign default values to the global structure */
haserl_t global = {
	.uploadkb = 2048,         /* how big an upload do we allow (0 for none) */
	.uploaddir = "/tmp",      /* where to upload to                         */
	.get = NULL,
	.post = NULL,
	.form = NULL,
	.cookie = NULL,
};

/* Convert 2 char hex string into char it represents
 * (from http://www.jmarshall.com/easy/cgi) */
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
		url[i] = x2c(&url[j + 1]);
		j += 2;
	}
	url[i] = 0;
}

/* if HTTP_COOKIE is passed as an environment variable,
 * attempt to parse its values into environment variables */
void
CookieVars(void)
{
	char *qs;
	char *token;

	if (getenv("HTTP_COOKIE")) {
		qs = strdup(getenv("HTTP_COOKIE"));
	} else {
		return;
	}

	/* split on; to extract name value pairs */
	token = strtok(qs, ";");
	while (token) {
		/* skip leading spaces */
		while (token[0] == ' ') {
			token++;
		}
		myputenv(&global.cookie, token);
		token = strtok(NULL, ";");
	}
	free(qs);
}

/* Read cgi variables from query string, and put in environment */
void
ReadCGIQueryString(void)
{
	char *qs;
	char *token;
	int i;

	if (getenv("QUERY_STRING")) {
		qs = strdup(getenv("QUERY_STRING"));
	} else {
		return;
	}

	/* change plusses into spaces */
	for (i = 0; qs[i]; i++) {
		if (qs[i] == '+') {
			qs[i] = ' ';
		}
	}

	/* split on & and ; to extract name value pairs */

	token = strtok(qs, "&;");
	while (token) {
		unescape_url(token);
		myputenv(&global.get, token);
		token = strtok(NULL, "&;");
	}
	free(qs);
}

/* Read cgi variables from stdin (for POST queries) */
void
ReadCGIPOSTValues(void)
{
	size_t content_length = 0;
	size_t max_len;
	int urldecoding = 0;
	char *matchstr = "";
	size_t i, j, x;
	sliding_buffer_t sbuf;
	buffer_t token;
	char *data;
	const char *CONTENT_LENGTH = "CONTENT_LENGTH";
	const char *CONTENT_TYPE = "CONTENT_TYPE";
	char *content_type = NULL;

	if (!getenv(CONTENT_LENGTH) ||
	    !strtoul(getenv(CONTENT_LENGTH), NULL, 10)) {
		return;
	}

	content_type = getenv(CONTENT_TYPE);

	if (content_type &&
	    !strncasecmp(content_type, "multipart/form-data", 19)) {
		/* This is a mime request, we need to go to the mime handler */
		rfc2388_handler();
		return;
	}

	/* at this point its either urlencoded or some other blob */
	if (!content_type ||
	    !strncasecmp(getenv(CONTENT_TYPE), "application/x-www-form-urlencoded", 33)) {
		urldecoding = 1;
		matchstr = "&";
	}

	max_len = global.uploadkb * 1024;

	s_buffer_init(&sbuf, 32768);
	sbuf.fh = 0;

	if (getenv(CONTENT_LENGTH)) {
		sbuf.maxread = strtoul(getenv(CONTENT_LENGTH), NULL, 10);
	}
	buffer_init(&token);

	if (!urldecoding) {
		buffer_add(&token, "body=", 5);
	}

	do {
		/* x is true if this token ends with a matchstr or is at the end of stream */
		x = s_buffer_read(&sbuf, matchstr);
		content_length += sbuf.len;
		if (content_length > max_len) {
			die(g_err_msg[E_OVER_LIMIT]);
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
				/* change plusses into spaces */
				j = strlen(data);
				for (i = 0; i <= j; i++) {
					if (data[i] == '+') {
						data[i] = ' ';
					}
				}
				unescape_url(data);
			}
			myputenv(&global.post, data);
			if (token.data) {
				buffer_reset(&token);
			}
		}
	} while (!sbuf.eof);
	s_buffer_destroy(&sbuf);
	buffer_destroy(&token);
}
