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

#ifndef _MAIN_H
#define _MAIN_H

/* how many argv slots to allocate at once */
#define ALLOC_CHUNK 10

/* name/value pairs */
typedef struct {
	char *string;  /* the string */
	int   quoted;  /* non-zero if the string was quoted */
} argv_t;

int argc_argv(char *instr, argv_t **argv, const char *commentstr);
void haserl_flags(void);
int ParseCommandLine(int argc, char * const *argv);
void BecomeUser(uid_t uid, gid_t gid);

#endif /* _MAIN_H */
