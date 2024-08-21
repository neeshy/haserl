#ifndef _SLIDING_BUF_H
#define _SLIDING_BUF_H

typedef struct {
	char    *buf;    /* the buffer */
	char    *ptr;    /* end of the valid data in the buffer */
	char    *limit;  /* end of the buffer */
	char    *begin;  /* beginning of the current segment */
	char    *end;    /* end of the current segment */
	char    *next;   /* beginning of the next segment */
	int      fd;     /* input file descriptor for the buffer */
	ssize_t  read;   /* number of bytes read from fd */
} sliding_buffer_t;

void s_buffer_init(sliding_buffer_t *sbuf, int fd, size_t size);
void s_buffer_destroy(sliding_buffer_t *sbuf);
int s_buffer_read(sliding_buffer_t *sbuf, const char *matchstr);

#endif /* _SLIDING_BUF_H */
