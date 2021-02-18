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

#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "h_error.h"

/* allocate memory or die, busybox style. */
void *
xmalloc(size_t size)
{
	void *buf;

	if (!(buf = malloc(size))) {
		die(g_err_msg[E_MALLOC_FAIL]);
	}
	memset(buf, 0, size);
	return buf;
}

/* realloc memory, or die xmalloc style. */
void *
xrealloc(void *buf, size_t size)
{
	if (!(buf = realloc(buf, size))) {
		die(g_err_msg[E_MALLOC_FAIL]);
	}
	return buf;
}

/* adds or replaces the "key=value" value in the env_list chain
 * prefix is appended to the key (e.g. FORM_key=value) */
void
myputenv(list_t **env, const char *str)
{
	list_t *cur = *env;
	list_t *prev = NULL;
	size_t slen;
	size_t keylen;
	char *entry = NULL;
	char *temp = NULL;
	int array = 0;

	slen = strlen(str);
	temp = memchr(str, '=', slen);
	/* if we don't have an equal sign, exit early */
	if (!temp) {
		return;
	}

	keylen = (size_t)(temp - str);

	/* is this an array */
	if (!memcmp(str + keylen - 2, "[]", 2)) {
		keylen = keylen - 2;
		array = 1;
	}

	if (array) {
		entry = xmalloc(slen - 1);
		strncat(entry, str, keylen);
		strcat(entry, str + keylen + 2);
	} else {
		entry = strdup(str);
		/* does the value already exist? */
		while (cur) {
			/* if found a matching key */
			if (!memcmp(cur->buf, entry, keylen)) {
				/* delete the old entry */
				free(cur->buf);
				if (prev) {
					prev->next = cur->next;
					free(cur);
					cur = prev->next;
				} else {
					free(cur);
					cur = NULL;
				}
			} else {
				prev = cur;
				cur = cur->next;
			}
		}
	}

	/* add the value to the end of the chain */
	cur = xmalloc(sizeof(list_t));
	cur->buf = entry;
	if (prev) {
		prev->next = cur;
	} else {
		*env = cur;
	}
}

/* free list_t */
void
free_list(list_t *list)
{
	list_t *next;

	while (list) {
		next = list->next;
		free(list->buf);
		free(list);
		list = next;
	}
}

/* Expandable Buffer is a reimplementation based on buffer.c in GCC
 * originally by Per Bother */
void
buffer_init(buffer_t *buf)
{
	buf->data = NULL;
	buf->ptr = NULL;
	buf->limit = NULL;
}

void
buffer_destroy(buffer_t *buf)
{
	if (buf->data) {
		free(buf->data);
	}
	buffer_init(buf);
}

/* don't reallocate - just forget about the current contents */
void
buffer_reset(buffer_t *buf)
{
	buf->ptr = buf->data;
}

void
buffer_add(buffer_t *buf, const void *data, size_t size)
{
	size_t newsize, index;

	/* if we need to grow the buffer, do so now */
	if (buf->ptr + size >= buf->limit) {
		index = (buf->limit - buf->data);
		newsize = index;
		while (newsize <= index + size) {
			newsize += 1024;
		}
		index = buf->ptr - buf->data;
		buf->data = xrealloc(buf->data, newsize);
		buf->limit = buf->data + newsize;
		buf->ptr = buf->data + index;
	}

	memcpy(buf->ptr, data, size);
	buf->ptr += size;
}
