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
