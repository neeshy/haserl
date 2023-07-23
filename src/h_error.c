#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "h_error.h"

char *g_err_msg[] = {
	"Memory Allocation Failure",
	"Unable to open file %s",
	"No script file specified",
	"Attempted to send content larger than allowed limits",
	"File uploads are not allowed",
	"No Mime Boundary Information Found",
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
