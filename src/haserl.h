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
void haserl_flags(list_t **env);
void CookieVars(list_t **env);
int ReadCGIQueryString(list_t **env);
int ReadCGIPOSTValues(list_t **env);
int LineToStr(char *string, size_t max);
int ReadMimeEncodedInput(list_t **env);
void PrintParseError(char *error, int linenum);
int ParseCommandLine(int argc, char *argv[]);
int BecomeUser(uid_t uid, gid_t gid);
void AssignGlobalStartupValues(void);

#endif /* _HASERL_H */
