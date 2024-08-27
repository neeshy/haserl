#ifndef _UTIL_H
#define _UTIL_H

#define append(ptr, elem, len, cap) \
	do { \
		if (len >= cap) { \
			cap += 8; \
			ptr = xrealloc(ptr, sizeof(elem) * cap); \
		} \
		ptr[len] = elem; \
		len++; \
	} while (0)

void *xmalloc(size_t size);
void *xrealloc(void *buf, size_t size);
char *xstrdup(const char *s);
void drain(int fd);
void die(const char *s, ...);
void die_status(int status, const char *s, ...);

#endif /* _UTIL_H */
