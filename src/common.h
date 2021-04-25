/* ---------------------------------------------------------------------------
 * Copyright 2003-2011 (inclusive) Nathan Angelacos
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

#ifndef _COMMON_H
#define _COMMON_H

/* linked list */
typedef struct {
	char *buf;
	void *next;
} list_t;

/* expandable buffer structure */
typedef struct {
	char *data;   /* the data */
	char *ptr;    /* where to write to next */
	char *limit;  /* maximal allocated buffer pos */
} buffer_t;

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

/* common.c */
void *xmalloc(size_t size);
void *xrealloc(void *buf, size_t size);
void myputenv(list_t **cur, const char *str);
void free_list(list_t *list);
void buffer_init(buffer_t *buf);
void buffer_reset(buffer_t *buf);
void buffer_destroy(buffer_t *buf);
void buffer_add(buffer_t *buf, const void *data, size_t size);

#endif /* _COMMON_H */
