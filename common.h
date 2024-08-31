#ifndef _COMMON_H
#define _COMMON_H

#include "util.h"

/* 128K */
#define CHUNK_SIZE 128 * 1024

typedef struct {
	size_t     upload_max;    /* maximum upload size (0 for none) */
	char      *upload_dir;    /* where we upload to               */
	lua_State *L;             /* lua state                        */
} haserl_t;

extern haserl_t global;

void haserl(void);
void multipart_handler(void);

void lua_init(void);
void lua_set(const char *tbl, const char *key, size_t key_size, const char *value, size_t value_size);
void lua_exec(const char *filename);

#endif /* _COMMON_H */
