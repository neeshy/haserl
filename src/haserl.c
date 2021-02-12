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
#include <sys/fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <grp.h>

#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#include "common.h"
#include "h_error.h"
#include "sliding_buffer.h"
#include "rfc2388.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "h_lua_common.h"
#ifdef INCLUDE_LUASHELL
#include "h_lua.h"
#endif
#ifdef INCLUDE_LUACSHELL
#include "h_luac.h"
#endif

#include "haserl.h"

#ifndef TEMPDIR
#define TEMPDIR "/tmp"
#endif

#ifndef MAX_UPLOAD_KB
#define MAX_UPLOAD_KB 2048
#endif

/* Refuse to disable the subshell */
#ifndef SUBSHELL_CMD
#define SUBSHELL_CMD "lua"
#endif

haserl_t global;

/* global shell execution function pointers. These point to the actual functions
 * that do the job, based on the language */

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
	{ "shell",          required_argument, 0, 's' },
	{ "silent",         no_argument,       0, 'S' },
	{ 0,                0,                 0, 0   }
};

const char *gs_short_options = "+vhu:U:H:ans:S";

/* Convert 2 char hex string into char it represents
 * (from http://www.jmarshall.com/easy/cgi) */
char
x2c(char *what)
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
	url[i] = '\0';
}

/* allocate memory or die, busybox style. */
void *
xmalloc(size_t size)
{
	void *buf;

	if ((buf = malloc(size)) == NULL) {
		die_with_message(g_err_msg[E_MALLOC_FAIL]);
	}
	memset(buf, 0, size);
	return buf;
}

/* realloc memory, or die xmalloc style. */
void *
xrealloc(void *buf, size_t size)
{
	if ((buf = realloc(buf, size)) == NULL) {
		die_with_message(g_err_msg[E_MALLOC_FAIL]);
	}
	return buf;
}

/* adds or replaces the "key=value" value in the env_list chain
 * prefix is appended to the key (e.g. FORM_key=value) */
list_t *
myputenv(list_t *cur, char *str, char *prefix)
{
	list_t *prev = NULL;
	size_t keylen, prefixlen;
	char *entry = NULL;
	char *temp = NULL;
	int array = 0;
	int len;

	temp = memchr(str, '=', strlen(str));
	/* if we don't have an equal sign, exit early */
	if (temp == 0) {
		return cur;
	}

	keylen = (size_t)(temp - str);

	/* is this an array */
	if (memcmp(str + keylen - 2, "[]", 2) == 0) {
		keylen = keylen - 2;
		array = 1;
	}

	entry = xmalloc(strlen(str) + strlen(prefix) + 1);
	entry[0] = '\0';
	if ((prefixlen = strlen(prefix))) {
		strncat(entry, prefix, prefixlen);
	}

	if (array == 1) {
		strncat(entry, str, keylen);
		strcat(entry, str + keylen + 2);
	} else {
		strcat(entry, str);
	}

	/* does the value already exist? */
	len = keylen + strlen(prefix) + 1;
	while (cur != NULL) {
		if (memcmp(cur->buf, entry, len) == 0) {
			if (array == 1) {
				/* if an array, create a new string with this
				 * value added to the end of the old value(s) */
				temp = xmalloc(strlen(cur->buf) + strlen(entry) - len + 2);
				memmove(temp, cur->buf, strlen(cur->buf) + 1);
				strcat(temp, "\n");
				strcat(temp, str + keylen + 3);
				free(entry);
				entry = temp;
			}
			/* delete the old entry */
			free(cur->buf);
			if (prev != NULL) {
				prev->next = cur->next;
			}
			free(cur);
			cur = prev;
		} /* end if found a matching key */
		prev = cur;
		if (cur) {
			cur = (list_t *)cur->next;
		}
	} /* end if matching key */

	/* add the value to the end of the chain */
	cur = xmalloc(sizeof(list_t));
	cur->buf = entry;
	if (prev != NULL) {
		prev->next = cur;
	}

	return cur;
}

/* free list_t chain */
void
free_list_chain(list_t *list)
{
	list_t *next;

	while (list) {
		next = list->next;
		free(list->buf);
		free(list);
		list = next;
	}
}

/* reads the current environment and popluates our environment chain */
void
readenv(list_t *env)
{
	extern char **environ;
	int count = 0;

	while (environ[count] != NULL) {
		myputenv(env, environ[count], global.nul_prefix);
		count++;
	}
}

/* if HTTP_COOKIE is passed as an environment variable,
 * attempt to parse its values into environment variables */
void
CookieVars(list_t *env)
{
	char *qs;
	char *token;

	if (getenv("HTTP_COOKIE") != NULL) {
		qs = strdup(getenv("HTTP_COOKIE"));
	} else {
		return;
	}

	/* split on; to extract name value pairs */
	token = strtok(qs, ";");
	while (token) {
		// skip leading spaces
		while (token[0] == ' ') {
			token++;
		}
		myputenv(env, token, global.var_prefix);
		myputenv(env, token, global.cookie_prefix);
		token = strtok(NULL, ";");
	}
	free(qs);
}

/* Makes a uniqe SESSIONID environment variable for this script */
void
sessionid(list_t *env)
{
	char session[29];

	sprintf(session, "SESSIONID=%x%x", getpid(), (int)time(NULL));
	myputenv(env, session, global.nul_prefix);
}

list_t *
wcversion(list_t *env)
{
	char version[200];

	sprintf(version, "HASERLVER=%s", PACKAGE_VERSION);
	return myputenv(env, version, global.nul_prefix);
}

void
haserlflags(list_t *env)
{
	char buf[200];

	snprintf(buf, 200, "UPLOAD_DIR=%s", global.uploaddir);
	myputenv(env, buf, global.haserl_prefix);

	snprintf(buf, 200, "UPLOAD_LIMIT=%lu", global.uploadkb);
	myputenv(env, buf, global.haserl_prefix);

	snprintf(buf, 200, "ACCEPT_ALL=%d", global.acceptall);
	myputenv(env, buf, global.haserl_prefix);

	snprintf(buf, 200, "SHELL=%s", global.shell);
	myputenv(env, buf, global.haserl_prefix);
}

/* Read cgi variables from query string, and put in environment */
int
ReadCGIQueryString(list_t *env)
{
	char *qs;
	char *token;
	int i;

	if (getenv("QUERY_STRING") != NULL) {
		qs = strdup(getenv("QUERY_STRING"));
	} else {
		return 0;
	}

	/* change plusses into spaces */
	for (i = 0; qs[i]; i++) {
		if (qs[i] == '+') {
			qs[i] = ' ';
		}
	}
	;

	/* split on & and ; to extract name value pairs */

	token = strtok(qs, "&;");
	while (token) {
		unescape_url(token);
		myputenv(env, token, global.var_prefix);
		myputenv(env, token, global.get_prefix);
		token = strtok(NULL, "&;");
	}
	free(qs);
	return 0;
}

/* Read cgi variables from stdin (for POST queries) */
int
ReadCGIPOSTValues(list_t *env)
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

	if ((getenv(CONTENT_LENGTH) == NULL) ||
	    (strtoul(getenv(CONTENT_LENGTH), NULL, 10) == 0)) {
		return 0;
	}

	content_type = getenv(CONTENT_TYPE);

	if ((content_type != NULL) &&
	    (strncasecmp(content_type, "multipart/form-data", 19)) == 0) {
		/* This is a mime request, we need to go to the mime handler */
		i = rfc2388_handler(env);
		return i;
	}

	/* at this point its either urlencoded or some other blob */
	if ((content_type == NULL) ||
	    (strncasecmp(getenv(CONTENT_TYPE), "application/x-www-form-urlencoded", 33) == 0)) {
		urldecoding = 1;
		matchstr = "&";
	}

	/* Allow 2MB content, unless they have a global upload set */
	max_len = ((global.uploadkb == 0) ? 2048 : global.uploadkb) * 1024;

	s_buffer_init(&sbuf, 32768);
	sbuf.fh = STDIN;

	if (getenv(CONTENT_LENGTH)) {
		sbuf.maxread = strtoul(getenv(CONTENT_LENGTH), NULL, 10);
	}
	haserl_buffer_init(&token);

	if (urldecoding == 0) {
		buffer_add(&token, "body=", 5);
	}

	do {
		/* x is true if this token ends with a matchstr or is at the end of stream */
		x = s_buffer_read(&sbuf, matchstr);
		content_length += sbuf.len;
		if (content_length > max_len) {
			die_with_message("Attempted to send content larger than allowed limits.");
		}

		if ((x == 0) || (token.data)) {
			buffer_add(&token, (char *)sbuf.segment, sbuf.len);
		}

		if (x) {
			data = sbuf.segment;
			sbuf.segment[sbuf.len] = '\0';
			if (token.data) {
				/* add the ASCIIZ */
				buffer_add(&token, sbuf.segment + sbuf.len, 1);
				data = token.data;
			}

			if (urldecoding) {
				/* change plusses into spaces */
				j = strlen((char *)data);
				for (i = 0; i <= j; i++) {
					if (data[i] == '+') {
						data[i] = ' ';
					}
				}
				unescape_url((char *)data);
			}
			myputenv(env, (char *)data, global.var_prefix);
			myputenv(env, (char *)data, global.post_prefix);
			if (token.data) {
				buffer_reset(&token);
			}
		}
	} while (!sbuf.eof);
	s_buffer_destroy(&sbuf);
	buffer_destroy(&token);
	return 0;
}

int
parseCommandLine(int argc, char *argv[])
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
		case 's':
			global.shell = optarg;
			break;
		case 'S':
			global.silent = TRUE;
			break;
		case 'u':
			if (optarg) {
				global.uploadkb = atoi(optarg);
			} else {
				global.uploadkb = MAX_UPLOAD_KB;
			}
			break;
		case 'a':
			global.acceptall = TRUE;
			break;
		case 'n':
			global.acceptall = NONE;
			break;
		case 'U':
			global.uploaddir = optarg;
			break;
		case 'H':
			global.uploadhandler = optarg;
			break;
		case 'v':
		case 'h':
			printf("This is " PACKAGE_NAME " version " PACKAGE_VERSION ""
			       " (http://haserl.sourceforge.net)\n");
			exit(0);
			break;
		}
	}
	return optind;
}

int
BecomeUser(uid_t uid, gid_t gid)
{
	/* This silently fails if it doesn't work */
	/* Following is from Timo Teras */
	if (getuid() == 0) {
		setgroups(1, &gid);
	}

	setgid(gid);
	setgid(getgid());

	setuid(uid);
	setuid(getuid());

	return 0;
}

/* Assign default values to the global structure */
void
assignGlobalStartupValues()
{
	global.uploadkb = 0;            /* how big an upload do we allow (0 for none) */
	global.shell = SUBSHELL_CMD;    /* The shell we use                           */
	global.silent = FALSE;          /* We do print errors if we find them         */
	global.uploaddir = TEMPDIR;     /* where to upload to                         */
	global.uploadhandler = NULL;    /* the upload handler                         */
	global.acceptall = FALSE;       /* don't allow POST data for GET method       */
	global.var_prefix = "FORM_";
	global.get_prefix = "GET_";
	global.post_prefix = "POST_";
	global.cookie_prefix = "COOKIE_";
	global.haserl_prefix = "HASERL_";
	global.nul_prefix = "";
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

	list_t *env = NULL;

	assignGlobalStartupValues();

	/* if more than argv[1] and argv[1] is not a file */
	switch (argc) {
	case 1:
		/* we were run, instead of called as a shell script */
		puts("This is " PACKAGE_NAME " version " PACKAGE_VERSION "\n"
		     "This program runs as a cgi interpeter, not interactively\n"
		     "Please see:  http://haserl.sourceforge.net\n"
		     "This version includes Lua (precompiled"
#ifdef INCLUDE_LUASHELL
		     " and interpreted"
#endif
		     ")\n"
		     );
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

		parseCommandLine(av2c, av2);
		free(av);
		if (av2 != argv) {
			free(av2);
		}

		if (optind < av2c) {
			filename = av2[optind];
		} else {
			die_with_message("No script file specified");
		}

		break;
	}

	/* drop permissions */
	stat(filename, &filestat);
	BecomeUser(filestat.st_uid, filestat.st_gid);

	/* populate the function pointers based on the shell selected */
	if (strcmp(global.shell, "lua") && strcmp(global.shell, "luac")) {
		die_with_message("Invalid shell specified.");
	} else {
		global.var_prefix = "FORM.";
		global.nul_prefix = "ENV.";
		global.get_prefix = "GET.";
		global.post_prefix = "POST.";
		global.cookie_prefix = "COOKIE.";
		global.haserl_prefix = "HASERL.";
		if (global.shell[3] == 'c') { /* luac only */
#ifndef INCLUDE_LUACSHELL
			die_with_message("Compiled Lua shell is not enabled.");
#endif
		} else {
#ifndef INCLUDE_LUASHELL
			die_with_message("Standard Lua shell is not enabled.");
#endif
		}
	}

	/* Read the current environment into our chain */
	env = wcversion(env);
	readenv(env);
	sessionid(env);
	haserlflags(env);

	/* Read the request data */
	if (global.acceptall != NONE) {
		/* If we have a request method, and we were run as a #! style script */
		CookieVars(env);
		if (getenv("REQUEST_METHOD")) {
			if ((strcasecmp(getenv("REQUEST_METHOD"), "GET") == 0) ||
			    (strcasecmp(getenv("REQUEST_METHOD"), "DELETE") == 0)) {
				if (global.acceptall == TRUE) {
					ReadCGIPOSTValues(env);
				}
				ReadCGIQueryString(env);
			}

			if ((strcasecmp(getenv("REQUEST_METHOD"), "POST") == 0) ||
			    (strcasecmp(getenv("REQUEST_METHOD"), "PUT") == 0)) {
				if (global.acceptall == TRUE) {
					ReadCGIQueryString(env);
				}
				ReadCGIPOSTValues(env);
			}
		}
	}

	lua_common_setup(global.shell, env);
#ifdef JUST_LUACSHELL
	luac_doscript(filename);
#else
	lua_doscript(filename);
#endif
	lua_common_destroy();

	free_list_chain(env);

	return 0;
}
