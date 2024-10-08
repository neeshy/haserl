#include <stdlib.h>
#include <string.h>

#include "util.h"

#include "buffer.h"

/* NOTE: this doesn't work when x (or y) is 0 */
#define ceildiv(x, y) (((x - 1) / y) + 1)

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
buffer_alloc(buffer_t *buf, size_t size)
{
	buf->data = xmalloc(size);
	buf->ptr = buf->data;
	buf->limit = buf->data + size;
}

/* don't reallocate - just forget about the current contents */
void
buffer_reset(buffer_t *buf)
{
	buf->ptr = buf->data;
}

void
buffer_destroy(buffer_t *buf)
{
	if (buf->data) {
		free(buf->data);
	}
	buffer_init(buf);
}

void
buffer_add(buffer_t *buf, const void *data, size_t size)
{
	/* if we need to grow the buffer, do so now */
	if (buf->ptr + size >= buf->limit) {
		size_t index = buf->ptr - buf->data;
		size_t newsize = buf->limit - buf->data + size;
		newsize = newsize ? ceildiv(newsize, ALLOC_SIZE) * ALLOC_SIZE : ALLOC_SIZE;
		buf->data = xrealloc(buf->data, newsize);
		buf->limit = buf->data + newsize;
		buf->ptr = buf->data + index;
	}

	memcpy(buf->ptr, data, size);
	buf->ptr += size;
}
