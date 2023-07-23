#include <stdlib.h>
#include <string.h>

#include "h_error.h"

#include "common.h"

/* Assign default values to the global structure */
haserl_t global = {
	.uploadkb = 2048,         /* how big an upload do we allow (0 for none) */
	.uploaddir = "/tmp",      /* where to upload to                         */
	.get = NULL,
	.post = NULL,
	.form = NULL,
	.cookie = NULL,
};

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

/* adds or replaces the "key=value" value in the env_list chain */
void
list_add(list_t **env, const char *str)
{
	list_t *cur = *env;
	list_t *prev = NULL;
	char *entry = NULL;
	char *temp = NULL;
	size_t keylen;

	temp = strchr(str, '=');
	/* if we don't have an equal sign, exit early */
	if (!temp) {
		return;
	}

	keylen = (size_t)(temp - str) + 1;
	entry = strdup(str);
	/* does the value already exist? */
	while (cur) {
		/* if found a matching key */
		if (!strncmp(cur->buf, entry, keylen)) {
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
list_destroy(list_t *list)
{
	list_t *next;

	while (list) {
		next = list->next;
		free(list->buf);
		free(list);
		list = next;
	}
}
