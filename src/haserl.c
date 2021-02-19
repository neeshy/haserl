/* --------------------------------------------------------------------------
 * Copyright 2003-2015 (inclusive) Nathan Angelacos
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
 * -----
 * The x2c() and unescape_url() routines were taken from
 *  http://www.jmarshall.com/easy/cgi/getcgi.c.txt
 *
 * The comments in that text file state:
 *
 ***  Written in 1996 by James Marshall, james@jmarshall.com, except
 ***  that the x2c() and unescape_url() routines were lifted directly
 ***  from NCSA's sample program util.c, packaged with their HTTPD.
 ***     For the latest, see http://www.jmarshall.com/easy/cgi/
 * -----
 *
 * ------------------------------------------------------------------------- */

#include <stdlib.h>
#include <string.h>

#include "libhaserl.h"

#include "haserl.h"

void
haserl(void)
{
	/* Read the request data */
	if (global.accept) {
		/* If we have a request method, and we were run as a #! style script */
		CookieVars();
		if (getenv("REQUEST_METHOD")) {
			if (!strcasecmp(getenv("REQUEST_METHOD"), "GET") ||
			    !strcasecmp(getenv("REQUEST_METHOD"), "DELETE")) {
				if (global.accept > 1) {
					ReadCGIPOSTValues();
				}
				ReadCGIQueryString();
			}

			if (!strcasecmp(getenv("REQUEST_METHOD"), "POST") ||
			    !strcasecmp(getenv("REQUEST_METHOD"), "PUT")) {
				if (global.accept > 1) {
					ReadCGIQueryString();
				}
				ReadCGIPOSTValues();
			}
		}
	}
}
