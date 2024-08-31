#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <grp.h>

#include <lua.h>
#include <lauxlib.h>

#include "common.h"
#include "haserl.h"

/*
 * split a string into an argv[] array, and return the number of elements.
 * Warning:  Overwrites s with nulls (to make ASCIIZ strings) and allocates
 * space for the argv array. The argv array will point to the offsets in
 * s where the elements occur. The calling function must free the argv
 * array, and should do so before freeing s.
 *
 * Example of use:
 *
 * int argc;
 * char **argv;
 * char string[1024];
 *
 * strcpy(string, "This\\ string will be \"separated into\" '6' elements.");
 * argc = split(string, &argv);
 * for (int i = 0; i < argc; i++) {
 * 	printf("%d: %s\n", i, argv[i]);
 * }
 * free(argv);
 */
static int
split(char *s, char ***ret)
{
	int slots = 0;
	int argc = 0;
	char **argv = NULL;

	char word = 0;
	while (*s) if (!word) switch (*s) {
		case '"':
		case '\'':
			word = *s;
			memmove(s, s + 1, strlen(s));
			append(argv, s, argc, slots);
			break;
		case '\\':
			switch (s[1]) {
			case '"':
			case '\'':
			case '\\':
			case ' ':
			case '\t':
			case '\r':
			case '\n':
				memmove(s, s + 1, strlen(s));
			}
		default:
			word = ' ';
			append(argv, s, argc, slots);
		case ' ':
		case '\t':
		case '\r':
		case '\n':
			s++;
	} else switch (*s) {
		case '"':
		case '\'':
			if (word == *s) {
				word = ' ';
				memmove(s, s + 1, strlen(s));
			} else if (word == ' ') {
				word = *s;
				memmove(s, s + 1, strlen(s));
			} else {
				s++;
			}
			break;
		case '\\':
			if (word == '"' && (s[1] == '"' || s[1] == '\\')) {
				memmove(s, s + 1, strlen(s));
			} else if (word == ' ') switch (s[1]) {
				case '"':
				case '\'':
				case '\\':
				case ' ':
				case '\t':
				case '\r':
				case '\n':
					memmove(s, s + 1, strlen(s));
			}
			s++;
			break;
		case ' ':
		case '\t':
		case '\r':
		case '\n':
			if (word == ' ') {
				word = 0;
				*s = 0;
			}
		default:
			s++;
	}

	*ret = argv;
	return argc;
}

int
main(int argc, char **argv)
{
	static struct option options[] = {
		{ "help",           no_argument,       NULL, 'h' },
		{ "version",        no_argument,       NULL, 'v' },
		{ "upload-limit",   required_argument, NULL, 'u' },
		{ "upload-dir",     required_argument, NULL, 'U' },
		{ NULL,             0,                 NULL, 0   },
	};

	int ac;
	char **av;
	if (argc == 1) {
		/* we were called directly instead of being called as a script */
		puts("This is " PACKAGE " version " VERSION "\n"
		     "This program runs as a CGI interpeter, not interactively\n"
		     "Please see: " URL);
		return 0;
	} else if (argc == 3) {
		/* split combined shebang args - linux bundles them as one */
		ac = split(argv[1], &av);
		av = xrealloc(av, sizeof(char *) * (ac + 2));
		memmove(av + 1, av, sizeof(char *) * ac);
		av[0] = argv[0];
		av[ac + 1] = argv[2];
		ac += 2;
	} else {
		av = argv;
		ac = argc;
	}

	/* set upload_dir to $TMPDIR before argument parsing */
	char *tmpdir = getenv("TMPDIR");
	if (tmpdir) {
		global.upload_dir = tmpdir;
	}

	int c;
	while ((c = getopt_long(ac, av, "+hvu:U:", options, NULL)) != -1) switch (c) {
		case 'u':
			global.upload_max = strtoul(optarg, NULL, 10) * 1024;
			break;
		case 'U':
			global.upload_dir = optarg;
			break;
		case 'v':
			puts(PACKAGE " version " VERSION " (" URL ")");
			return 0;
		case 'h':
		case '?':
			puts("Usage: " PACKAGE " [-v|--version] [-U dirspec|--upload-dir=dirspec] [-u limit|--upload-limit=limit] [--] FILENAME");
			return c != 'h';
	}

	if (optind >= ac) {
		die("No script file specified");
	}
	char *filename = av[optind];

	if (av != argv) {
		free(av);
	}

	/* drop permissions */
	struct stat filestat;
	if (!getuid() && !stat(filename, &filestat)) {
		/* these calls will silently fail if they don't work */
		setgroups(0, NULL);
		setgid(filestat.st_gid);
		setuid(filestat.st_uid);
	}

	lua_init();
	haserl();
	lua_exec(filename);
	lua_close(global.L);

	return 0;
}
