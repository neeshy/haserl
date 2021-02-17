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

/* how many argv slots to allocate at once */
#define ALLOC_CHUNK 10

int argc_argv(char *instr, argv_t **argv, char *commentstr);
char x2c(char *what);
void unescape_url(char *url);
void haserl_flags(void);
void CookieVars(void);
void ReadCGIQueryString(void);
void ReadCGIPOSTValues(void);
int ParseCommandLine(int argc, char *argv[]);
void BecomeUser(uid_t uid, gid_t gid);

#endif /* _HASERL_H */
