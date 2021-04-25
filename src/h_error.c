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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "h_error.h"

char *g_err_msg[] = {
	"",
	"Memory Allocation Failure",
	"Unable to open file %s",
	"Unable to start shell",
	"No script file specified",
	"Attempted to send content larger than allowed limits",
	"File uploads are not allowed",
	"No Mime Boundary Information Found",
	"Unknown operation",
	"Unspecified Error",
};

/* print an error message and die. If sp or where are non-null pointers, then
 * a line is added saying where in the script buffer the error occured. If
 * there's a request method, then http headers are added. */
void
die(const char *s, ...)
{
	va_list p;
	FILE *fo = stderr;

	if (getenv("REQUEST_METHOD")) {
		fo = stdout;
		fprintf(fo, "HTTP/1.0 500 Server Error\r\n"
		        "Content-Type: text/html\r\n\r\n"
		        "<html><body><b><font color='#C00'>" PACKAGE
		        " CGI Error</font></b><br><pre>\r\n");
	}

	va_start(p, s);
	vfprintf(fo, s, p);
	va_end(p);
	printf("\r\n");

	if (getenv("REQUEST_METHOD")) {
		fprintf(fo, "</pre></body></html>");
	}

	exit(-1);
}
