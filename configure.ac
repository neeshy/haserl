# Process this file with autoconf to produce a configure script.
AC_INIT([haserl],[0.9.35],[Nathan Angelacos - nangel@users.sourceforge.net],[haserl])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

# allow different compiler for build tools in case cross compiling
if test -z "$CC_FOR_BUILD"; then
	CC_FOR_BUILD="$CC"
fi

# allow different compiler switches
if test -z "$CFLAGS_FOR_BUILD"; then
	CFLAGS_FOR_BUILD="$CFLAGS"
fi

AC_SUBST(CC_FOR_BUILD)
AC_SUBST(CFLAGS_FOR_BUILD)

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CONFIG_HEADERS(src/config.h)
AC_CHECK_HEADERS([stdlib.h string.h unistd.h signal.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MEMCMP
AC_CHECK_FUNCS([memset putenv strcasecmp strdup strncasecmp strstr])

AC_DEFINE([_GNU_SOURCE], [], [Enable GNU Extensions])

dnl **************************************************************
dnl Checks for lua

luashell=true
luacshell=false

AC_ARG_WITH([lua],
	AS_HELP_STRING([--with-lua], [Specify lua pkg-config name (default is 'lua')]))

AS_IF([test "x$with_lua" = "xyes"], [with_lua=lua])
AS_IF([test "x$with_lua" != "xno" -a "x$with_lua" != "x"], [
	luashell=true
	luacshell=true
	PKG_CHECK_MODULES([LUA], [$with_lua])
])

# If Lua is enabled, the user can choose between two different shells

# shell: ordinary Haserl code with embedded Lua
AC_ARG_ENABLE(luashell,
	AS_HELP_STRING([--enable-luashell],[Includes the standard Lua shell - Haserl with embedded Lua (default is yes if Lua's enabled)]),
[case "${enableval}" in
	yes) luashell=true ;;
	no)  luashell=false ;;
	*)   AC_MSG_ERROR(bad value ${enableval} for --enable-luashell) ;;
esac], [])
AM_CONDITIONAL(INCLUDE_LUASHELL, test x$luashell = xtrue)
if test x$luashell = xtrue; then
	AC_DEFINE(INCLUDE_LUASHELL, , [Include ordinary Lua shell])
fi

# shell: precompiled Haserl code - compiled Lua code
AC_ARG_ENABLE(luacshell,
	AS_HELP_STRING([--enable-luacshell],[Includes the compiled Lua shell - precompiled Haserl/Lua (default is yes if Lua's enabled)]),
[case "${enableval}" in
	yes) luacshell=true ;;
	no)  luacshell=false ;;
	*)   AC_MSG_ERROR(bad value ${enableval} for --enable-luacshell) ;;
esac], [])
AM_CONDITIONAL(INCLUDE_LUACSHELL, test x$luacshell = xtrue)
if test x$luacshell = xtrue; then
	AC_DEFINE(INCLUDE_LUACSHELL, , [Include shell for precompiled Haserl/Lua])
fi

# set additional macros

if test x$luashell = xfalse; then
	if test x$luacshell = xtrue; then
		AC_DEFINE(JUST_LUACSHELL, , [Include just the compiled Lua shell])
	else
		AC_MSG_ERROR([All shells disabled.])
	fi
fi

# What default shell should we use
AC_ARG_ENABLE(defaultshell,
	[ --enable-defaultshell=cmd - enable the default shell to use (default is lua) ])

# Handle "--disable-defaultshell"
case $enable_defaultshell in
	no )	enable_defaultshell=
		;;
	"" )	enable_defaultshell="lua"
		;;
	esac

AC_DEFINE_UNQUOTED( [DEFAULT_SHELL], "$enable_defaultshell",
	[the default shell to start up])

CFLAGS="$CFLAGS -Wall"

AC_CONFIG_FILES([Makefile
		src/Makefile
		doc/Makefile
		])

AC_OUTPUT

dnl  report configuration
AC_MSG_RESULT([
** Configuration summary for $PACKAGE $VERSION:

	Building haserl with luashell=$luashell
	Building haserl with luacshell=$luacshell
])
