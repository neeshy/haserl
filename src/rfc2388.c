/* ---------------------------------------------------------------------------
 * Copyright 2003-2014 (inclusive) Nathan Angelacos
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

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "common.h"
#include "h_error.h"
#include "sliding_buffer.h"
#include "rfc2388.h"

void
empty_stdin(void)
{
	char c[2048];
	while (read(0, &c, sizeof(c)));
}

void
mime_var_init(mime_var_t *obj)
{
	obj->name = NULL;
	obj->filename = NULL;
	obj->type = NULL;
	obj->tempname = NULL;
	buffer_init(&obj->value);
	obj->fh = 0;
}

void
mime_var_destroy(mime_var_t *obj)
{
	int status;
	struct sigaction new_action;

	if (obj->name) {
		free(obj->name);
		obj->name = NULL;
	}
	if (obj->filename) {
		free(obj->filename);
		obj->filename = NULL;
	}
	if (obj->type) {
		free(obj->type);
		obj->type = NULL;
	}
	if (obj->tempname) {
		free(obj->tempname);
		obj->tempname = NULL;
	}
	buffer_destroy(&obj->value);
	if (obj->fh) {
		close(abs(obj->fh));
		if (global.uploadhandler) {
			wait(&status);
			new_action.sa_handler = SIG_DFL;
			sigemptyset(&new_action.sa_mask);
			new_action.sa_flags = 0;
			sigaction(SIGPIPE, &new_action, NULL);
		}
		obj->fh = 0;
	}
}

char *
mime_substr(char *start, int len)
{
	char *ptr;

	if (!start || len < 0) {
		return NULL;
	}
	ptr = xmalloc(len + 2);
	memcpy(ptr, start, len);
	return ptr;
}

void
mime_tag_add(mime_var_t *obj, char *str)
{
	char *a = NULL;
	char *b = NULL;
	static char *tag[] = { "name=\"", "filename=\"", "Content-Type: " };

	a = strcasestr(str, tag[0]);
	if (a) {
		a += strlen(tag[0]);
		b = strchr(a, '"');
		if (!obj->name && b) {
			obj->name = mime_substr(a, b - a);
		}
	}

	a = strcasestr(str, tag[1]);
	if (a) {
		a += strlen(tag[1]);
		b = strchr(a, '"');
		if (!obj->filename && b) {
			obj->filename = mime_substr(a, b - a);
		}
	}

	a = strcasestr(str, tag[2]);
	if (a) {
		a += strlen(tag[2]);
		b = a + strlen(a);
		if (!obj->type) {
			obj->type = mime_substr(a, b - a);
		}
	}
}

void
mime_var_putenv(mime_var_t *obj)
{
	buffer_t buf;

	buffer_init(&buf);
	if (obj->name) {
		/* For file uploads, this creates FORM_foo=tempfile_pathspec.
		 * That name can be overwritten by a subsequent foo=/etc/passwd,
		 * for instance. This code block is depricated for FILE uploads
		 * only. (it is still valid for non-form uploads */
		buffer_add(&obj->value, "", 1);
		buffer_add(&buf, obj->name, strlen(obj->name));
		buffer_add(&buf, "=", 1);
		buffer_add(&buf, obj->value.data,
		           strlen(obj->value.data) + 1);
		myputenv(&global.post, buf.data);
		buffer_reset(&buf);
	}
	if (obj->filename) {
		/* This creates HASERL_foo_path=tempfile_pathspec. */
		buffer_add(&buf, obj->name, strlen(obj->name));
		buffer_add(&buf, "_path=", 6);
		buffer_add(&buf, obj->value.data,
		           strlen(obj->value.data) + 1);
		myputenv(&global.haserl, buf.data);
		buffer_reset(&buf);

		/* this saves the name of the file the client supplied */
		buffer_add(&buf, obj->name, strlen(obj->name));
		buffer_add(&buf, "_filename=", 10);
		buffer_add(&buf, obj->filename, strlen(obj->filename) + 1);
		myputenv(&global.haserl, buf.data);
		buffer_reset(&buf);
	}
	buffer_destroy(&buf);
}

void
mime_exec(mime_var_t *obj, int fd)
{
	int i;
	long limit;
	char *av[2];
	char *type, *filename, *name;
	struct sigaction new_action;

	switch (fork()) {
	case -1:
		empty_stdin();
		die(g_err_msg[E_SHELL_FAIL]);
		break;

	case 0:
		/* store the content type, filename, and form name */
		if (obj->type) {
			type = xmalloc(13 + strlen(obj->type) + 1);
			sprintf(type, "CONTENT_TYPE=%s", obj->type);
			putenv(type);
		}

		if (obj->filename) {
			filename = xmalloc(9 + strlen(obj->filename) + 1);
			sprintf(filename, "FILENAME=%s", obj->filename);
			putenv(filename);
		}

		if (obj->name) {
			name = xmalloc(5 + strlen(obj->name) + 1);
			sprintf(name, "NAME=%s", obj->name);
			putenv(name);
		}

		if (fd) {
			close(0);
			dup2(fd, 0);
		}

		if ((limit = sysconf(_SC_OPEN_MAX)) == -1) {
			limit = 1024;
		}

		for (i = 1; i < limit; i++) {
			close(i);
		}

		open("/dev/null", O_RDWR);
		dup2(1, 2);

		av[0] = global.uploadhandler;
		av[1] = NULL;
		execv(av[0], av);
		/* if we get here, we had a failure. Not much we can do.
		 * We are the child, so we can't even warn the parent */
		empty_stdin();
		exit(-1);
		break;

	default:
		/* I'm parent - ignore SIGPIPE from the child */
		new_action.sa_handler = SIG_IGN;
		sigemptyset(&new_action.sa_mask);
		new_action.sa_flags = 0;
		sigaction(SIGPIPE, &new_action, NULL);
		break;

	}
	/* control should get to this point only in the parent. */
} /* end mime_exec */

void
mime_var_open_target(mime_var_t *obj)
{
	char *tmpname;
	int fd[2];
	int ok;

	/* if upload_limit is zero, we die right here */
	if (!global.uploadkb) {
		empty_stdin();
		die(g_err_msg[E_FILE_UPLOAD]);
	}

	/* reuse the name as a fifo if we have a handler. We do this
	 * because tempnam uses TEMPDIR if defined, among other bugs */
	if (global.uploadhandler) {
		/* I have a handler */
		ok = !pipe(fd);
		if (ok) {
			mime_exec(obj, fd[0]);
			close(fd[0]);
			obj->fh = fd[1];
		}
	} else {
		tmpname = xmalloc(strlen(global.uploaddir) + 8);
		strcpy(tmpname, global.uploaddir);
		strcat(tmpname, "/XXXXXX");
		obj->fh = mkstemp(tmpname);
		ok = obj->fh != -1;
		if (ok) {
			buffer_add(&obj->value, tmpname, strlen(tmpname));
		}
	}

	if (!ok) {
		empty_stdin();
		die(g_err_msg[E_FILE_OPEN_FAIL], tmpname);
	}
}

void
mime_var_writer(mime_var_t *obj, char *str, int len)
{
	int err;

	/* if not a file upload, then just a normal variable */
	if (!obj->filename) {
		buffer_add(&obj->value, str, len);
	}

	/* if a file upload, but don't have an open filehandle, open one */
	if (!obj->fh && obj->filename) {
		mime_var_open_target(obj);
	}

	/* if we have an open file, write the chunk */
	if (obj->fh > 0) {
		err = write(obj->fh, str, len);
		/* if there was an error, invert the filehandle; we need the
		 * handle for later when we close it */
		if (err == -1) {
			obj->fh = abs(obj->fh) * -1;
		}
	}
}

/* Read multipart/form-data input (RFC2388), typically used when
 * uploading a file. */
void
rfc2388_handler(void)
{
	enum mime_state_t { DISCARD, BOUNDARY, HEADER, CONTENT };

	int state;
	int i, x;
	unsigned long max_len, content_length;
	sliding_buffer_t sbuf;
	char *crlf = "\r\n";
	char *boundary;
	char *str;
	buffer_t buf;
	mime_var_t var;

	int header_continuation;

	/* prevent a potential unitialized free() - ISE-TPS-2014-008 */
	var.name = NULL;

	/* get the boundary info */
	str = getenv("CONTENT_TYPE");
	i = strlen(str) - 9;
	while (i >= 0 && memcmp("boundary=", str + i, 9)) {
		i--;
	}
	if (i == -1) {
		empty_stdin();
		die(g_err_msg[E_MIME_BOUNDARY]);
	}

	i = i + 9;
	if (str[i] == '"') {
		i++;
	}

	boundary = xmalloc(strlen(str + i) + 5); /* \r\n-- + NULL */
	memcpy(boundary, crlf, 2);
	memcpy(boundary + 2, "--", 2);
	memcpy(boundary + 4, str + i, strlen(str + i) + 1);
	if (i > 0 && str[i - 1] == '"') {
		while (boundary[i] && boundary[i] != '"') {
			i++;
		}
		boundary[i] = 0;
	}

	/* Allow 2MB content, unless they have a global upload set */
	max_len = (!global.uploadkb ? MAX_UPLOAD_KB : global.uploadkb) * 1024;
	content_length = 0;

	/* initialize a 128K sliding buffer */
	s_buffer_init(&sbuf, 1024 * 128);
	sbuf.fh = 0;
	if (getenv("CONTENT_LENGTH")) {
		sbuf.maxread = strtoul(getenv("CONTENT_LENGTH"), NULL, 10);
	}

	/* initialize the buffer, and make sure it doesn't point to null */
	buffer_init(&buf);
	buffer_add(&buf, "", 1);
	buffer_reset(&buf);

	state = DISCARD;
	str = boundary + 2; /* skip the leading crlf */

	header_continuation = 0;

	do {
		/* x is true if this token ends with a matchstr or is at the end of stream */
		x = s_buffer_read(&sbuf, str);
		content_length += sbuf.len;
		if (content_length >= max_len) {
			empty_stdin();
			free(boundary);
			s_buffer_destroy(&sbuf);
			buffer_destroy(&buf);
			if (var.name) {
				mime_var_destroy(&var);
			}
			die(g_err_msg[E_OVER_LIMIT]);
		}

		switch (state) {
		case DISCARD:
			/* discard any text - used for first mime boundary */
			if (x) {
				state = BOUNDARY;
				str = crlf;
				buffer_reset(&buf); /* reinitializes the buffer */
			}
			break;

		case BOUNDARY:
			buffer_add(&buf, sbuf.segment, sbuf.len);
			if (x) {
				if (!memcmp(buf.data, boundary + 2, 2)) { /* "--" */
					/* all done... what does that mean? */
					str = boundary + 2;
					state = DISCARD;
				} else {
					buffer_reset(&buf);
					mime_var_init(&var);
					state = HEADER;
					header_continuation = 0;
					str = crlf;
				}
			}
			break;

		case HEADER:
			buffer_add(&buf, sbuf.segment, sbuf.len);
			if (x) {
				if (!sbuf.len && !header_continuation) { /* blank line */
					buffer_reset(&buf);
					state = CONTENT;
					str = boundary;
				} else {
					buffer_add(&buf, "", 1);
					mime_tag_add(&var, buf.data);
					buffer_reset(&buf);
				}
				header_continuation = 0;
			} else {
				/* expect more data */
				header_continuation = 1;
			}
			break;

		case CONTENT:
			/* write to writer process, regardless */
			mime_var_writer(&var, sbuf.segment, sbuf.len);
			if (x) {
				buffer_reset(&buf);
				mime_var_putenv(&var);
				mime_var_destroy(&var);
				state = BOUNDARY;
				str = crlf;
			}
			break;

		} /* end switch */
	} while (!sbuf.eof);
	free(boundary);
	s_buffer_destroy(&sbuf);
	buffer_destroy(&buf);
}
