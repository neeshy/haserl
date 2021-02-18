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

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <grp.h>

#include "common.h"
#include "h_error.h"
#include "sliding_buffer.h"
#include "rfc2388.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "h_lua.h"

#include "haserl.h"

/* Assign default values to the global structure */
haserl_t global = {
	.uploadkb = 0,            /* how big an upload do we allow (0 for none) */
	.uploaddir = TEMPDIR,     /* where to upload to                         */
	.uploadhandler = NULL,    /* the upload handler                         */
	.get = NULL,
	.post = NULL,
	.cookie = NULL,
	.haserl = NULL,
	.accept = 1,              /* don't allow POST data for GET method       */
	.silent = 0               /* we do print errors if we find them         */
};

/* Command line / Config file directives When adding a long option, make sure
 * to update the short_options as well */
struct option ga_long_options[] = {
	{ "version",        no_argument,       0, 'v' },
	{ "help",           no_argument,       0, 'h' },
	{ "upload-limit",   required_argument, 0, 'u' },
	{ "upload-dir",     required_argument, 0, 'U' },
	{ "upload-handler", required_argument, 0, 'H' },
	{ "accept-all",     no_argument,       0, 'a' },
	{ "accept-none",    no_argument,       0, 'n' },
	{ "silent",         no_argument,       0, 'S' },
	{ 0,                0,                 0, 0   }
};

const char *gs_short_options = "+vhu:U:H:anS";

/*
 * split a string into an argv[] array, and return the number of elements.
 * Warning:  Overwrites instr with nulls (to make ASCIIZ strings) and mallocs
 * space for the argv array. The argv array will point to the offsets in
 * instr where the elements occur. The calling function must free the argv
 * array, and should do so before freeing instr memory.
 *
 * If comment points to a non-null string, then any character in the string
 * will mark the beginning of a comment until the end-of-line:
 *
 * comment="#;"
 * foo bar     # This is a comment
 * foo baz     ; This is a comment as well
 *
 * Example of use:
 *
 * int argc, count; argv_t *argv; char string[2000];
 *
 * strcpy(string, "This\\ string will be \"separated into\" '6' elements.", "");
 * argc = argc_argv(string, &argv);
 * for (count = 0; count < argc; count++) {
 * 	printf ("%03d: %s\n", count, argv[count].string);
 * }
 * free(argv);
 */
int
argc_argv(char *instr, argv_t **argv, const char *commentstr)
{
	char quote = '\0';
	int arg_count = 0;
	enum state_t {
		WHITESPACE, WORDSPACE, TOKENSTART
	} state = WHITESPACE;
	argv_t *argv_array = NULL;
	int argc_slots = 0;
	size_t len, pos;

	len = strlen(instr);
	pos = 0;
	char quoted = 0;

	while (pos < len) {
		/* Comments are really, really special */
		if (state == WHITESPACE && strchr(commentstr, *instr)) {
			while (*instr != '\n' && *instr != '\0') {
				instr++;
				pos++;
			}
		}

		switch (*instr) {
		/* quoting */
		case '"':
		case '\'':
			/* Begin quoting */
			if (state == WHITESPACE) {
				quote = *instr;
				state = TOKENSTART;
				quoted = -1;
				if (*(instr + 1) == quote) { /* special case for NULL quote */
					*instr = '\0';
				} else {
					instr++;
					pos++;
				}
			} else { /* WORDSPACE, so quotes end or quotes within quotes */
				/* Is it the same kind of quote? */
				if (*instr == quote && quoted) {
					quote = '\0';
					*instr = '\0';
					state = WHITESPACE;
				}
			}
			break;

		/* backslash - if escaping a quote within a quote */
		case '\\':
			if (quote && *(instr + 1) == quote) {
				memmove(instr, instr + 1, strlen(instr));
				len--;
			}
			/* otherwise, its just a normal character */
			else {
				if (state == WHITESPACE) {
					state = TOKENSTART;
				}
			}
			break;

		/* whitepsace */
		case ' ':
		case '\t':
		case '\r':
		case '\n':
			if (state == WORDSPACE && quote == '\0') {
				state = WHITESPACE;
				*instr = '\0';
			}
			break;

		case '\0':
			break;

		default:
			if (state == WHITESPACE) {
				state = TOKENSTART;
			}
		} /* end switch */

		if (state == TOKENSTART) {
			arg_count++;
			if (arg_count > argc_slots) {
				argc_slots += ALLOC_CHUNK;
				argv_array = xrealloc(argv_array, sizeof(argv_t) * (argc_slots + ALLOC_CHUNK));
			}

			if (!argv_array) {
				return -1;
			}
			argv_array[arg_count - 1].string = instr;
			argv_array[arg_count - 1].quoted = quoted;
			state = WORDSPACE;
			quoted = 0;
		}

		instr++;
		pos++;
	}

	if (!arg_count) return 0;

	argv_array[arg_count].string = NULL;
	*argv = argv_array;
	return arg_count;
}

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

void
haserl_flags(void)
{
	char buf[256];

	putenv("HASERLVER=" VERSION);

	snprintf(buf, 256, "SESSIONID=%x%x", getpid(), (int)time(NULL));
	putenv(buf);

	snprintf(buf, 256, "HASERL_UPLOAD_DIR=%s", global.uploaddir);
	putenv(buf);

	snprintf(buf, 256, "HASERL_UPLOAD_LIMIT=%lu", global.uploadkb);
	putenv(buf);

	snprintf(buf, 256, "HASERL_ACCEPT_ALL=%d", global.accept);
	putenv(buf);
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
	unsigned char *data;
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

	/* Allow 2MB content, unless they have a global upload set */
	max_len = (!global.uploadkb ? MAX_UPLOAD_KB : global.uploadkb) * 1024;

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

int
ParseCommandLine(int argc, char * const *argv)
{
	int c;
	int option_index = 0;

	/* set optopt and optind to 0 to reset getopt_long -
	 * we may call it multiple times */
	optopt = 0;
	optind = 0;

	while ((c = getopt_long(argc, argv, gs_short_options,
	                        ga_long_options, &option_index)) != -1) {
		switch (c) {
		case 'S':
			global.silent = 1;
			break;
		case 'u':
			global.uploadkb = atoi(optarg);
			break;
		case 'a':
			global.accept = 2;
			break;
		case 'n':
			global.accept = 0;
			break;
		case 'U':
			global.uploaddir = optarg;
			break;
		case 'H':
			global.uploadhandler = optarg;
			break;
		case 'v':
		case 'h':
			printf("This is " PACKAGE " version " VERSION
			       " (http://haserl.sourceforge.net)\n");
			exit(0);
			break;
		}
	}
	return optind;
}

void
BecomeUser(uid_t uid, gid_t gid)
{
	/* This silently fails if it doesn't work */
	/* Following is from Timo Teras */
	if (!getuid()) {
		setgroups(1, &gid);
	}

	setgid(gid);
	setgid(getgid());

	setuid(uid);
	setuid(getuid());
}

int
main(int argc, char *argv[])
{
	char *filename = NULL;
	struct stat filestat;

	argv_t *av = NULL;
	char **av2 = argv;
	int av2c = argc;

	int command;
	int count;

	/* if more than argv[1] and argv[1] is not a file */
	switch (argc) {
	case 1:
		/* we were run, instead of called as a shell script */
		puts("This is " PACKAGE " version " VERSION "\n"
		     "This program runs as a cgi interpeter, not interactively\n"
		     "Please see:  http://haserl.sourceforge.net\n"
		     "This version includes Lua (precompiled and interpreted)\n");
		return 0;
		break;
	default: /* more than one */
		/* split combined #! args - linux bundles them as one */
		command = argc_argv(argv[1], &av, "");

		if (command > 1) {
			/* rebuild argv into new av2 */
			av2c = argc - 1 + command;
			av2 = xmalloc(sizeof(char *) * av2c);
			av2[0] = argv[0];
			for (count = 1; count <= command; count++) {
				av2[count] = av[count - 1].string;
			}
			for (; count < av2c; count++) {
				av2[count] = argv[count - command + 1];
			}
		}

		ParseCommandLine(av2c, av2);
		free(av);
		if (av2 != argv) {
			free(av2);
		}

		if (optind < av2c) {
			filename = av2[optind];
		} else {
			die(g_err_msg[E_NO_SCRIPT_FILE]);
		}

		break;
	}

	/* drop permissions */
	stat(filename, &filestat);
	BecomeUser(filestat.st_uid, filestat.st_gid);

	/* Add environment variables */
	haserl_flags();

	/* Read the request data */
	if (global.accept) {
		/* If we have a request method, and we were run as a #! style script */
		CookieVars();
		if (getenv("REQUEST_METHOD")) {
			if (!strcasecmp(getenv("REQUEST_METHOD"), "GET") ||
			    !strcasecmp(getenv("REQUEST_METHOD"), "DELETE")) {
				if (global.accept > 1) {
					ReadCGIPOSTValues();
				}
				ReadCGIQueryString();
			}

			if (!strcasecmp(getenv("REQUEST_METHOD"), "POST") ||
			    !strcasecmp(getenv("REQUEST_METHOD"), "PUT")) {
				if (global.accept > 1) {
					ReadCGIQueryString();
				}
				ReadCGIPOSTValues();
			}
		}
	}

	lua_setup();
	lua_doscript(filename);
	lua_destroy();

	free_list(global.get);
	free_list(global.post);
	free_list(global.cookie);
	free_list(global.haserl);

	return 0;
}
