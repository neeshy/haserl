#ifndef _SLIDING_BUF_H
#define _SLIDING_BUF_H

/* sliding buffer structure */
typedef struct {
	int     fh;       /* the input filehandle for the buffer */
	char   *buf;      /* pointer to the buffer */
	char   *ptr;      /* start positon (used internally) */
	char   *segment;  /* the start position of this segment */
	size_t  len;      /* length of this segment */
	size_t  maxsize;  /* max size of buffer */
	size_t  bufsize;  /* current size of buffer */
	size_t  maxread;  /* maximum number of bytes to read from fh, ignored if 0 */
	size_t  nrread;   /* number of bytes read from fh */
	int     eof;      /* true if there is no more to read */
} sliding_buffer_t;

/* sliding_buffer.c */
void s_buffer_init(sliding_buffer_t *sbuf, int size);
void s_buffer_destroy(sliding_buffer_t *sbuf);
int s_buffer_read(sliding_buffer_t *sbuf, const char *matchstr);

#endif /* _SLIDING_BUF_H */
