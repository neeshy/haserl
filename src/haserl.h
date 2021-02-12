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

/* Just a silly construct to contain global variables    */
typedef struct {
	unsigned long  uploadkb;       /* how big an upload do we allow (0 for none)*/
	char          *shell;          /* The shell we use                          */
	char          *uploaddir;      /* where we upload to                        */
	char          *uploadhandler;  /* a handler for uploads                     */
	char          *var_prefix;     /* what name we give to FORM variables       */
	char          *get_prefix;     /* what name we give to GET variables        */
	char          *post_prefix;    /* what name we give to POST variables       */
	char          *cookie_prefix;  /* what name we give to COOKIE variables     */
	char          *nul_prefix;     /* what name we give to environment variables*/
	char          *haserl_prefix;  /* what name we give to HASERL variables     */
	int            acceptall;      /* true if we'll accept POST data on
	                                *      GETs and vice versa                  */
	int            silent;         /* true if we never print errors             */
} haserl_t;

extern haserl_t global;

char x2c(char *what);
void unescape_url(char *url);
void *xmalloc(size_t size);
void *xrealloc(void *buf, size_t size);
list_t *myputenv(list_t *cur, char *str, char *prefix);
void free_list_chain(list_t *);
void readenv(list_t *env);
void CookieVars(list_t *env);
void sessionid(list_t *env);
list_t *wcversion(list_t *env);
void haserlflags(list_t *env);
int ReadCGIQueryString(list_t *env);
int ReadCGIPOSTValues(list_t *env);
int LineToStr(char *string, size_t max);
int ReadMimeEncodedInput(list_t *env);
void PrintParseError(char *error, int linenum);
int parseCommandLine(int argc, char *argv[]);
int BecomeUser(uid_t uid, gid_t gid);
void assignGlobalStartupValues(void);
void unlink_uploadlist(void);
int main(int argc, char *argv[]);

#endif /* _HASERL_H */
