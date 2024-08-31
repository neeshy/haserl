#ifndef _BUFFER_H
#define _BUFFER_H

#define ALLOC_SIZE 1024

typedef struct {
	char *data;   /* the data */
	char *ptr;    /* where to write to next */
	char *limit;  /* maximal allocated buffer pos */
} buffer_t;

void buffer_init(buffer_t *buf);
void buffer_alloc(buffer_t *buf, size_t size);
void buffer_reset(buffer_t *buf);
void buffer_destroy(buffer_t *buf);
void buffer_add(buffer_t *buf, const void *data, size_t size);

#endif /* _BUFFER_H */
