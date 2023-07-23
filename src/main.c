#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <grp.h>

#include "haserl.h"
#include "common.h"
#include "h_error.h"

#include "h_lua.h"

/* how many argv slots to allocate at once */
#define ALLOC_CHUNK 10

/* name/value pairs */
typedef struct {
	char *string;  /* the string */
	int   quoted;  /* non-zero if the string was quoted */
} argv_t;

/* Command line / Config file directives When adding a long option, make sure
 * to update the short_options as well */
struct option ga_long_options[] = {
	{ "version",        no_argument,       0, 'v' },
	{ "help",           no_argument,       0, 'h' },
	{ "upload-limit",   required_argument, 0, 'u' },
	{ "upload-dir",     required_argument, 0, 'U' },
	{ 0,                0,                 0, 0   }
};

const char *gs_short_options = "+vhu:U:";

/*
 * split a string into an argv[] array, and return the number of elements.
 * Warning:  Overwrites instr with nulls (to make ASCIIZ strings) and mallocs
 * space for the argv array. The argv array will point to the offsets in
 * instr where the elements occur. The calling function must free the argv
 * array, and should do so before freeing instr memory.
 *
 * If comment points to a non-null string, then any character in the string
 * will mark the beginning of a comment until the end-of-line:
 *
 * comment="#;"
 * foo bar     # This is a comment
 * foo baz     ; This is a comment as well
 *
 * Example of use:
 *
 * int argc, count; argv_t *argv; char string[2000];
 *
 * strcpy(string, "This\\ string will be \"separated into\" '6' elements.", "");
 * argc = argc_argv(string, &argv);
 * for (count = 0; count < argc; count++) {
 * 	printf ("%03d: %s\n", count, argv[count].string);
 * }
 * free(argv);
 */
int
argc_argv(char *instr, argv_t **argv, const char *commentstr)
{
	char quote = '\0';
	int arg_count = 0;
	enum state_t {
		WHITESPACE, WORDSPACE, TOKENSTART
	} state = WHITESPACE;
	argv_t *argv_array = NULL;
	int argc_slots = 0;
	size_t len, pos;

	len = strlen(instr);
	pos = 0;
	char quoted = 0;

	while (pos < len) {
		/* Comments are really, really special */
		if (state == WHITESPACE && strchr(commentstr, *instr)) {
			while (*instr != '\n' && *instr != '\0') {
				instr++;
				pos++;
			}
		}

		switch (*instr) {
		/* quoting */
		case '"':
		case '\'':
			/* Begin quoting */
			if (state == WHITESPACE) {
				quote = *instr;
				state = TOKENSTART;
				quoted = -1;
				if (*(instr + 1) == quote) { /* special case for NULL quote */
					*instr = '\0';
				} else {
					instr++;
					pos++;
				}
			} else { /* WORDSPACE, so quotes end or quotes within quotes */
				/* Is it the same kind of quote? */
				if (*instr == quote && quoted) {
					quote = '\0';
					*instr = '\0';
					state = WHITESPACE;
				}
			}
			break;

		/* backslash - if escaping a quote within a quote */
		case '\\':
			if (quote && *(instr + 1) == quote) {
				memmove(instr, instr + 1, strlen(instr));
				len--;
			}
			/* otherwise, its just a normal character */
			else {
				if (state == WHITESPACE) {
					state = TOKENSTART;
				}
			}
			break;

		/* whitepsace */
		case ' ':
		case '\t':
		case '\r':
		case '\n':
			if (state == WORDSPACE && quote == '\0') {
				state = WHITESPACE;
				*instr = '\0';
			}
			break;

		case '\0':
			break;

		default:
			if (state == WHITESPACE) {
				state = TOKENSTART;
			}
		} /* end switch */

		if (state == TOKENSTART) {
			arg_count++;
			if (arg_count > argc_slots) {
				argc_slots += ALLOC_CHUNK;
				argv_array = xrealloc(argv_array, sizeof(argv_t) * (argc_slots + ALLOC_CHUNK));
			}

			if (!argv_array) {
				return -1;
			}
			argv_array[arg_count - 1].string = instr;
			argv_array[arg_count - 1].quoted = quoted;
			state = WORDSPACE;
			quoted = 0;
		}

		instr++;
		pos++;
	}

	if (!arg_count) return 0;

	argv_array[arg_count].string = NULL;
	*argv = argv_array;
	return arg_count;
}

int
ParseCommandLine(int argc, char * const *argv)
{
	int c;
	int option_index = 0;

	/* set optopt and optind to 0 to reset getopt_long -
	 * we may call it multiple times */
	optopt = 0;
	optind = 0;

	while ((c = getopt_long(argc, argv, gs_short_options,
	                        ga_long_options, &option_index)) != -1) {
		switch (c) {
		case 'u':
			global.uploadkb = atoi(optarg);
			break;
		case 'U':
			global.uploaddir = optarg;
			break;
		case 'v':
		case 'h':
			printf("This is " PACKAGE " version " VERSION
			       " (http://haserl.sourceforge.net)\n");
			exit(0);
			break;
		}
	}
	return optind;
}

void
BecomeUser(uid_t uid, gid_t gid)
{
	/* This silently fails if it doesn't work */
	/* Following is from Timo Teras */
	if (!getuid()) {
		setgroups(1, &gid);
	}

	setgid(gid);
	setgid(getgid());

	setuid(uid);
	setuid(getuid());
}

int
main(int argc, char *argv[])
{
	char *filename = NULL;
	struct stat filestat;

	argv_t *av = NULL;
	char **av2 = argv;
	int av2c = argc;

	int command;
	int count;

	/* if more than argv[1] and argv[1] is not a file */
	if (argc == 1) {
		/* we were run, instead of called as a shell script */
		puts("This is " PACKAGE " version " VERSION "\n"
		     "This program runs as a cgi interpeter, not interactively\n"
		     "Please see:  http://haserl.sourceforge.net\n"
		     "This version includes Lua (precompiled and interpreted)");
		return 0;
	}

	/* more than one */
	/* split combined #! args - linux bundles them as one */
	command = argc_argv(argv[1], &av, "");

	if (command > 1) {
		/* rebuild argv into new av2 */
		av2c = argc - 1 + command;
		av2 = xmalloc(sizeof(char *) * av2c);
		av2[0] = argv[0];
		for (count = 1; count <= command; count++) {
			av2[count] = av[count - 1].string;
		}
		for (; count < av2c; count++) {
			av2[count] = argv[count - command + 1];
		}
	}

	ParseCommandLine(av2c, av2);
	free(av);
	if (av2 != argv) {
		free(av2);
	}

	if (optind < av2c) {
		filename = av2[optind];
	} else {
		die(g_err_msg[E_NO_SCRIPT_FILE]);
	}

	/* drop permissions */
	stat(filename, &filestat);
	BecomeUser(filestat.st_uid, filestat.st_gid);

	haserl();

	lua_doscript(filename);

	list_destroy(global.get);
	list_destroy(global.post);
	list_destroy(global.form);
	list_destroy(global.cookie);

	return 0;
}
