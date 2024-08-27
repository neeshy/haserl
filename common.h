#ifndef _COMMON_H
#define _COMMON_H

#define append(ptr, elem, len, cap) \
	do { \
		if (len >= cap) { \
			cap += 8; \
			ptr = xrealloc(ptr, sizeof(elem) * cap); \
		} \
		ptr[len] = elem; \
		len++; \
	} while (0)

/* 128K */
#define CHUNK_SIZE 128 * 1024

typedef struct {
	size_t    upload_max;     /* maximum upload size (0 for none)        */
	char     *upload_dir;     /* where we upload to                      */
	buffer_t  get;            /* name-value pairs for GET requests       */
	buffer_t  post;           /* name-value pairs for POST requests      */
	buffer_t  form;           /* name-value pairs for the FORM namespace */
	buffer_t  cookie;         /* name-value pairs for cookie headers     */
} haserl_t;

extern haserl_t global;

void *xmalloc(size_t size);
void *xrealloc(void *buf, size_t size);
char *xstrdup(const char *s);
void haserl_destroy(haserl_t *haserl);
void drain(int fd);
void die(const char *s, ...);
void die_status(int status, const char *s, ...);

#endif /* _COMMON_H */
