#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "buffer.h"
#include "sliding_buffer.h"
#include "common.h"

#include "multipart.h"

typedef struct {
	char     *name;           /* the variable name */
	char     *filename;       /* the client-specified filename */
	buffer_t  value;          /* the value of the variable */
	int       fd;             /* the output file descriptor */
} form_data_t;

static void
form_data_init(form_data_t *obj)
{
	obj->name = NULL;
	obj->filename = NULL;
	buffer_init(&obj->value);
	obj->fd = -1;
}

static void
form_data_destroy(form_data_t *obj)
{
	if (obj->name) {
		free(obj->name);
		obj->name = NULL;
	}
	if (obj->filename) {
		free(obj->filename);
		obj->filename = NULL;
	}
	buffer_destroy(&obj->value);
	if (obj->fd != -1) {
		if (obj->fd < -1) {
			obj->fd = -obj->fd - 2;
		}
		close(obj->fd);
		obj->fd = -1;
	}
}

/* read multipart/form-data input (RFC2388), typically used when uploading a file. */
void
multipart_handler(void)
{
	/* get the boundary info */
	char *str = getenv("CONTENT_TYPE");
	if (!str || !(str = strcasestr(str, "boundary="))) {
		die("No boundary information found");
	}

	size_t len;
	char *quote;
	str += 9;
	if (*str == '"' && (quote = strchr(str++, '"'))) {
		len = quote - str;
	} else {
		len = strlen(str);
	}

	char *boundary = xmalloc(len + 5);
	memcpy(boundary, "\r\n--", 4);
	memcpy(boundary + 4, str, len);
	boundary[len + 5] = 0;

	sliding_buffer_t sbuf;
	s_buffer_init(&sbuf, 0, CHUNK_SIZE);

	/* initialize a buffer and make sure it doesn't point to null */
	buffer_t buf;
	buffer_init(&buf);
	buffer_add(&buf, NULL, 0);

	/* prevent a potential unitialized free() - ISE-TPS-2014-008 */
	form_data_t form_data;
	form_data_init(&form_data);

	enum { DISCARD, BOUNDARY, HEADER, CONTENT } state = DISCARD;
	str = boundary + 2; /* skip the leading CRLF initially */

	size_t read = 0;
	while (sbuf.read != -1) {
		int matched = s_buffer_read(&sbuf, str);
		if (matched == -1) {
			free(boundary);
			s_buffer_destroy(&sbuf);
			buffer_destroy(&buf);
			form_data_destroy(&form_data);
			die("Provided multipart boundary exceeds size of internal buffer");
		}

		read += sbuf.read;
		if (read > global.upload_max) {
			free(boundary);
			s_buffer_destroy(&sbuf);
			buffer_destroy(&buf);
			form_data_destroy(&form_data);
			die("Reached maximum allowable input length");
		}

		switch (state) {
		case DISCARD:
			/* discard any text - used for first boundary */
			if (matched) {
				state = BOUNDARY;
				str = "\r\n";
			}
			break;

		case BOUNDARY:
			buffer_add(&buf, sbuf.begin, sbuf.end - sbuf.begin);
			if (matched) {
				if (!memcmp(buf.data, "--", 2)) {
					/* all done */
					drain(sbuf.fd);
					/* manually set EOF */
					sbuf.read = -1;
				} else {
					form_data_init(&form_data);
					state = HEADER;
					str = "\r\n";
				}
				buffer_reset(&buf);
			}
			break;

		case HEADER:
			buffer_add(&buf, sbuf.begin, sbuf.end - sbuf.begin);
			if (matched) {
				/* blank line, end of header */
				if (!(sbuf.end - sbuf.begin)) {
					buffer_reset(&buf);
					if (form_data.name) {
						state = CONTENT;
					} else {
						/* if no name was given, ignore this part */
						form_data_destroy(&form_data);
						state = DISCARD;
					}
					str = boundary;
					continue;
				}

				buffer_add(&buf, "", 1);

				if (!strncasecmp(buf.data, "Content-Disposition:", 20)) {
					char *s;
					if (!form_data.name && ((s = strcasestr(buf.data, " name=\"")) || (s = strcasestr(buf.data, ";name=\"")))) {
						s += 7;
						char *q = strchr(s, '"');
						if (q) {
							size_t len = q - s;
							form_data.name = xmalloc(len + 1);
							memcpy(form_data.name, s, len);
							form_data.name[len] = 0;
						}
					}

					if (!form_data.filename && ((s = strcasestr(buf.data, " filename=\"")) || (s = strcasestr(buf.data, ";filename=\"")))) {
						s += 11;
						char *q = strchr(s, '"');
						if (q) {
							size_t len = q - s;
							form_data.filename = xmalloc(len + 1);
							memcpy(form_data.filename, s, len);
							form_data.filename[len] = 0;

							if (form_data.fd == -1) {
								/* if a file upload, but don't have an open fd, open one */
								char *tmpname = xmalloc(strlen(global.upload_dir) + 8);
								strcpy(tmpname, global.upload_dir);
								strcat(tmpname, "/XXXXXX");
								if ((form_data.fd = mkstemp(tmpname)) == -1) {
									free(boundary);
									s_buffer_destroy(&sbuf);
									buffer_destroy(&buf);
									form_data_destroy(&form_data);
									die_status(errno, "mkstemp: %s: %s", tmpname, strerror(errno));
								}
								buffer_add(&form_data.value, tmpname, strlen(tmpname));
								free(tmpname);
							}
						}
					}
				}

				buffer_reset(&buf);
			}
			break;

		case CONTENT:
			if (!form_data.filename) {
				/* if not a file upload, populate the value field */
				buffer_add(&form_data.value, sbuf.begin, sbuf.end - sbuf.begin);
			} else if (form_data.fd > -1) {
				/* if we have an open file, write the chunk
				 * if there was an error, invert the file descriptor
				 * we need the descriptor later when we close it */
				if (write(form_data.fd, sbuf.begin, sbuf.end - sbuf.begin) == -1) {
					form_data.fd = -form_data.fd - 2;
				}
			}

			if (matched) {
				if (form_data.filename) {
					/* this creates FORM.foo_path=tempfile_pathspec. */
					buffer_add(&global.form, form_data.name, strlen(form_data.name));
					buffer_add(&global.form, "_path", 6);
					buffer_add(&global.form, form_data.value.data, strlen(form_data.value.data) + 1);

					/* this saves the name of the file the client supplied */
					buffer_add(&global.form, form_data.name, strlen(form_data.name));
					buffer_add(&global.form, "_filename", 10);
					buffer_add(&global.form, form_data.filename, strlen(form_data.filename) + 1);
				} else {
					buffer_add(&global.post, form_data.name, strlen(form_data.name) + 1);
					buffer_add(&global.post, form_data.value.data, form_data.value.ptr - form_data.value.data);
					buffer_add(&global.post, "", 1);
				}
				form_data_destroy(&form_data);
				state = BOUNDARY;
				str = "\r\n";
			}
			break;

		}
	}

	free(boundary);
	s_buffer_destroy(&sbuf);
	buffer_destroy(&buf);
	form_data_destroy(&form_data);
}
