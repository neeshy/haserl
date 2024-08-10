#ifndef _COMMON_H
#define _COMMON_H

/* linked list */
typedef struct {
	char *buf;
	void *next;
} list_t;

/* Just a silly construct to contain global variables */
typedef struct {
	unsigned long  uploadkb;       /* how big an upload do we allow (0 for none) */
	char          *uploaddir;      /* where we upload to                         */
	list_t        *get;            /* name-value pairs for GET requests          */
	list_t        *post;           /* name-value pairs for POST requests         */
	list_t        *form;           /* name-value pairs for the FORM namespace    */
	list_t        *cookie;         /* name-value pairs for cookie headers        */
} haserl_t;

extern haserl_t global;

void *xmalloc(size_t size);
void *xrealloc(void *buf, size_t size);
char *xstrdup(const char *s);
void die(const char *s, ...);
void list_add(list_t **cur, const char *str);
void list_destroy(list_t *list);

#endif /* _COMMON_H */
