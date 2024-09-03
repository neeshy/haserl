#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "util.h"

#include "sliding_buffer.h"

void
s_buffer_init(sliding_buffer_t *sbuf, int fd, size_t size)
{
	/* NUL terminate the buffer so it can be safely used as a string */
	sbuf->buf = xmalloc(size + 1);
	sbuf->buf[size] = 0;
	sbuf->ptr = sbuf->buf;
	sbuf->limit = sbuf->buf + size;
	sbuf->begin = sbuf->buf;
	sbuf->end = sbuf->buf;
	sbuf->next = sbuf->buf;
	sbuf->fd = fd;
	sbuf->read = 0;
}

void
s_buffer_destroy(sliding_buffer_t *sbuf)
{
	free(sbuf->buf);
	sbuf->buf = NULL;
	sbuf->ptr = NULL;
	sbuf->limit = NULL;
	sbuf->begin = NULL;
	sbuf->end = NULL;
	sbuf->next = NULL;
	sbuf->fd = -1;
	sbuf->read = 0;
}

/* read the next segment from a sliding buffer
 * returns 1 if the next segment contains a matchstr token
 * returns 0 if there is no match after the current segment
 * returns -1 if we are at the end the file or if there were any read errors */
int
s_buffer_read(sliding_buffer_t *sbuf, const char *matchstr)
{
	/* if EOF and next ran off the buffer, then we are done */
	if (sbuf->read == -1 && sbuf->next >= sbuf->ptr) return -1;

	size_t matchlen = matchstr ? strlen(matchstr) : 0;
	/* a malicious client can send a matchstr longer than the actual content body
	 * do not allow reads beyond the buffer limits */
	if (sbuf->limit - matchlen < sbuf->buf) return -1;

	/* if we need to fill the buffer, do so now */
	char *begin = sbuf->next;
	char *limit = sbuf->ptr - matchlen;
	if (begin >= limit) {
		/* shift contents of buffer
		 * this discards anything before sbuf->next */
		size_t len = sbuf->ptr - begin;
		memmove(sbuf->buf, begin, len);
		begin = sbuf->buf;
		sbuf->ptr = begin + len;

		/* if fd is invalid, we are at EOF
		 * pigeonhole errors and EOF */
		if (fcntl(sbuf->fd, F_GETFL) != -1 && (sbuf->read = read(sbuf->fd, sbuf->ptr, sbuf->limit - sbuf->ptr)) > 0) {
			sbuf->ptr += sbuf->read;
		} else {
			sbuf->read = -1;
		}
		limit = sbuf->ptr - matchlen;
	} else {
		sbuf->read = 0;
	}

	/* if we have a matchstr, look for it */
	sbuf->begin = begin;
	/* in case of partial or null reads, limit may be too small */
	if (matchlen && begin + matchlen <= limit) {
		char *ptr = begin;
		while (ptr <= limit) {
			if (!memcmp(ptr, matchstr, matchlen)) {
				/* skip matchstr */
				sbuf->end = ptr;
				sbuf->next = ptr + matchlen;
				return 1;
			}
			ptr++;
		}
	}

	/* if there is no more input left to read, return the entire buffer */
	if (sbuf->read == -1) {
		sbuf->end = sbuf->ptr;
		sbuf->next = sbuf->ptr;
		return 0;
	}

	/* return the current segment sans the length of the matchstr
	 * searching all of the available input while only returning segments up to
	 *   (ptr - matchlen)
	 * ensures subsequent searches are correct even when the matchstr spans two
	 * segments */
	if (limit < begin) limit = begin;
	sbuf->end = limit;
	sbuf->next = limit;
	return 0;
}
