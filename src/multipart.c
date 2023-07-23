#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "h_error.h"
#include "buffer.h"
#include "sliding_buffer.h"

#include "multipart.h"

typedef struct {
	char     *name;           /* the variable name */
	char     *filename;       /* the client-specified filename */
	char     *type;           /* the mime-type */
	char     *tempname;       /* the tempfilename */
	buffer_t  value;          /* the value of the variable */
	int       fh;             /* the output file handle */
} mime_var_t;

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
		obj->fh = 0;
	}
}

char *
mime_substr(const char *start, int len)
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
mime_tag_add(mime_var_t *obj, const char *str)
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
mime_var_list_add(mime_var_t *obj)
{
	buffer_t buf;

	buffer_init(&buf);
	if (obj->name) {
		if (obj->filename) {
			/* This creates FORM.foo_path=tempfile_pathspec. */
			buffer_add(&buf, obj->name, strlen(obj->name));
			buffer_add(&buf, "_path=", 6);
			buffer_add(&buf, obj->value.data,
				   strlen(obj->value.data) + 1);
			list_add(&global.form, buf.data);
			buffer_reset(&buf);

			/* this saves the name of the file the client supplied */
			buffer_add(&buf, obj->name, strlen(obj->name));
			buffer_add(&buf, "_filename=", 10);
			buffer_add(&buf, obj->filename, strlen(obj->filename) + 1);
			list_add(&global.form, buf.data);
			buffer_reset(&buf);
		} else {
			buffer_add(&obj->value, "", 1);
			buffer_add(&buf, obj->name, strlen(obj->name));
			buffer_add(&buf, "=", 1);
			buffer_add(&buf, obj->value.data,
				   strlen(obj->value.data) + 1);
			list_add(&global.post, buf.data);
			buffer_reset(&buf);
		}
	}
	buffer_destroy(&buf);
}

/* Read multipart/form-data input (RFC2388), typically used when
 * uploading a file. */
void
multipart_handler(void)
{
	enum mime_state_t { DISCARD, BOUNDARY, HEADER, CONTENT };

	int state;
	int i, x;
	unsigned long max_len, content_length;
	sliding_buffer_t sbuf;
	char *crlf = "\r\n";
	char *boundary;
	char *str;
	char *tmpname;
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

	boundary = xmalloc(strlen(str + i) + 5); /* \r\n--\0 */
	memcpy(boundary, crlf, 2);
	memcpy(boundary + 2, "--", 2);
	memcpy(boundary + 4, str + i, strlen(str + i) + 1);
	if (i > 0 && str[i - 1] == '"') {
		while (boundary[i] && boundary[i] != '"') {
			i++;
		}
		boundary[i] = 0;
	}

	max_len = global.uploadkb * 1024;
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
			/* write to writer process, regardless
			 * if not a file upload, then just a normal variable */
			if (!var.filename) {
				buffer_add(&var.value, sbuf.segment, sbuf.len);
			} else if (!var.fh) { /* if a file upload, but don't have an open filehandle, open one */
				/* if upload_limit is zero, we die right here */
				if (!global.uploadkb) {
					empty_stdin();
					die(g_err_msg[E_FILE_UPLOAD]);
				}

				/* reuse the name as a fifo if we have a handler. We do this
				 * because tempnam uses TEMPDIR if defined, among other bugs */
				tmpname = xmalloc(strlen(global.uploaddir) + 8);
				strcpy(tmpname, global.uploaddir);
				strcat(tmpname, "/XXXXXX");
				if ((var.fh = mkstemp(tmpname)) != -1) {
					buffer_add(&var.value, tmpname, strlen(tmpname));
				} else {
					empty_stdin();
					die(g_err_msg[E_FILE_OPEN_FAIL], tmpname);
				}
			}
			/* if we have an open file, write the chunk */
			if (var.fh > 0) {
				/* if there was an error, invert the filehandle; we need the
				 * handle for later when we close it */
				if (write(var.fh, sbuf.segment, sbuf.len) == -1) {
					var.fh = abs(var.fh) * -1;
				}
			}
			if (x) {
				buffer_reset(&buf);
				mime_var_list_add(&var);
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
