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

#ifndef _H_ERROR_H
#define _H_ERROR_H

enum error_types {
	E_NO_ERROR,
	E_MALLOC_FAIL,
	E_FILE_OPEN_FAIL,
	E_SHELL_FAIL,
	E_NO_SCRIPT_FILE,
	E_OVER_LIMIT,
	E_FILE_UPLOAD,
	E_MIME_BOUNDARY,
	E_NO_OP,
	E_WHATEVER,
};

extern char *g_err_msg[];

/* h_error.c */
void die(const char *s, ...);

#endif /* _H_ERROR_H */
