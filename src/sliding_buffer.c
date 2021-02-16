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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "sliding_buffer.h"
#include "common.h"
#include "h_error.h"

/* initialize a sliding buffer structure */
void
s_buffer_init(sliding_buffer_t *sbuf, int size)
{
	sbuf->fh = 0; /* use stdin by default */
	sbuf->maxsize = size;
	sbuf->buf = xmalloc(sbuf->maxsize);
	sbuf->ptr = sbuf->buf;
	sbuf->segment = NULL;
	/* reduce maxsize by one, so that you can add a NULL to the end of any
	 * returned token and not have a memory overwrite */
	sbuf->maxsize -= 1;
	sbuf->len = 0;
	sbuf->bufsize = 0;
	sbuf->maxread = 0;
	sbuf->nrread = 0;
	sbuf->eof = 0;
}

/* destroy a sliding buffer structure */
void
s_buffer_destroy(sliding_buffer_t *sbuf)
{
	free(sbuf->buf);
}

/* read the next segment from a sliding buffer. returns !=0 if the
 * segment ends at a matchstr token, or if we are at the end of the string
 * returns 0 if the segment does not end */
int
s_buffer_read(sliding_buffer_t *sbuf, char *matchstr)
{
	int len, pos;
	int r;

	/* if eof and ptr ran off the buffer, then we are done */
	if ((sbuf->eof) && (sbuf->ptr > sbuf->buf)) {
		return 0;
	}

	/* if need to fill the buffer, do so */
	if ((sbuf->bufsize == 0) ||
	    (sbuf->ptr >= (sbuf->buf + sbuf->bufsize - strlen(matchstr)))) {
		len = sbuf->bufsize - (sbuf->ptr - sbuf->buf);
		if (len) {
			memmove(sbuf->buf, sbuf->ptr, len);
		}
		sbuf->ptr = sbuf->buf;
		sbuf->bufsize = len;
		/* if the filedescriptor is invalid, we are obviously
		 * at an end of file condition. */
		if (fcntl(sbuf->fh, F_GETFL) == -1) {
			r = 0;
		} else {
			size_t n = sbuf->maxsize - len;
			if (sbuf->maxread && sbuf->maxread < sbuf->nrread + n) {
				n = sbuf->maxread - sbuf->nrread;
			}
			r = read(sbuf->fh, sbuf->buf + len, n);
		}
		/* only report eof when we've done a read of 0. */
		if (r == 0 || (r < 0 && errno != EINTR)) {
			sbuf->eof = -1;
		} else {
			sbuf->bufsize += (r > 0) ? r : 0;
			sbuf->nrread += (r > 0) ? r : 0;
		}
	}

	/* look for the matchstr */
	pos = 0;
	len = sbuf->bufsize - (int)(sbuf->ptr - sbuf->buf) - strlen(matchstr);
	/* a malicious client can send a matchstr longer than the actual content body
	 * do not allow reads beyond the buffer limits */
	len = (len < 0) ? 0 : len;

	/* if have a matchstr, look for it, otherwise return the chunk */
	if (strlen(matchstr) > 0) {
		while (memcmp(matchstr, sbuf->ptr + pos, strlen(matchstr)) && (pos < len)) {
			pos++;
		}

		/* if we found it */
		if (pos < len) {
			sbuf->len = pos;
			sbuf->segment = sbuf->ptr;
			sbuf->ptr = sbuf->segment + pos + strlen(matchstr);
			return -1;
		}

		if (sbuf->eof) {
			len += strlen(matchstr);
		}
	}
	/* ran off the end, didn't find the matchstr */
	sbuf->segment = sbuf->ptr;
	sbuf->len = len;
	sbuf->ptr += sbuf->len;
	return sbuf->eof ? -1 : 0;
}
