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

#ifndef _HASERL_H
#define _HASERL_H

/* linked list */
typedef struct {
	char *buf;
	void *next;
} list_t;

/* Just a silly construct to contain global variables */
typedef struct {
	unsigned long  uploadkb;       /* how big an upload do we allow (0 for none) */
	char          *uploaddir;      /* where we upload to                         */
	char          *uploadhandler;  /* a handler for uploads                      */
	list_t        *get;            /* name-value pairs for GET requests          */
	list_t        *post;           /* name-value pairs for POST requests         */
	list_t        *form;           /* name-value pairs for the FORM namespace    */
	list_t        *cookie;         /* name-value pairs for cookie headers        */
	int            accept;         /* true if we'll accept POST data on
	                                * GETs and vice versa                        */
	int            silent;         /* true if we never print errors              */
} haserl_t;

extern haserl_t global;

void haserl(void);

#endif /* _HASERL_H */
