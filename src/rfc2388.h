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

#ifndef _RFC2388_H
#define _RFC2388_H

typedef struct {
	char     *name;           /* the variable name */
	char     *filename;       /* the client-specified filename */
	char     *type;           /* the mime-type */
	char     *tempname;       /* the tempfilename */
	buffer_t  value;          /* the value of the variable */
	int       fh;             /* the output file handle */
} mime_var_t;

/* rfc2388.c */
void empty_stdin(void);
void mime_var_init(mime_var_t *obj);
void mime_var_destroy(mime_var_t *obj);
char *mime_substr(const char *start, int len);
void mime_tag_add(mime_var_t *obj, const char *str);
void mime_var_putenv(mime_var_t *obj);
void mime_exec(mime_var_t *obj, int fd);
void mime_var_open_target(mime_var_t *obj);
void mime_var_writer(mime_var_t *obj, const char *str, int len);
void rfc2388_handler(void);

#endif /* _RFC2388_H */
