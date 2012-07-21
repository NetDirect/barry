#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# Parts taken from the pkg.m4 file from pkg-config
# Copyright 2004 Scott James Remnant <scott@netsplit.com>
#

AC_PREREQ(2.61)
AC_INIT([barrydesktop], [0.19.0], [barry-devel@lists.sourceforge.net])
#AM_CONFIG_HEADER(config.h)
AC_CONFIG_SRCDIR([src/barrydesktop.cc])
AC_CONFIG_HEADERS([config.h:config.h.in])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

AM_GNU_GETTEXT([external])
# this is the verion of gettext, not barry
AM_GNU_GETTEXT_VERSION([0.18.1])

#
# Barry Desktop Version Numbers
#
# Be sure to keep BARRY_DESKTOP_MAJOR == BARRY_MAJOR from ../configure.ac!
# (see desktop/src/Makefile.am for the reason)
#
BARRY_DESKTOP_LOGICAL=0
BARRY_DESKTOP_MAJOR=19
BARRY_DESKTOP_MINOR=0
AC_DEFINE_UNQUOTED([BARRY_DESKTOP_LOGICAL], [$BARRY_DESKTOP_LOGICAL], [Logical version number])
AC_DEFINE_UNQUOTED([BARRY_DESKTOP_MAJOR], [$BARRY_DESKTOP_MAJOR], [Major library version number])
AC_DEFINE_UNQUOTED([BARRY_DESKTOP_MINOR], [$BARRY_DESKTOP_MINOR], [Minor library version number])
AC_DEFINE_UNQUOTED([BARRY_DESKTOP_VER_STRING],
	["${BARRY_DESKTOP_LOGICAL}.${BARRY_DESKTOP_MAJOR}.${BARRY_DESKTOP_MINOR}"],
	[Full Barry Desktop version in string form])
AC_SUBST([BARRY_DESKTOP_LOGICAL])
AC_SUBST([BARRY_DESKTOP_MAJOR])
AC_SUBST([BARRY_DESKTOP_MINOR])

#
# Checks for programs.
#
AC_PROG_CC
AC_PROG_CXX
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

AC_LANG([C])

PKG_CHECK_MODULES([LIBGCAL], [libgcal >= 0.9.6], [],
	[
	LGTEMP_LDFLAGS="$LDFLAGS"
	LDFLAGS="$LDFLAGS -lgcal"
	AC_RUN_IFELSE(
		[AC_LANG_PROGRAM(
			[[
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int get_the_url(char *data, int length, char **url);

void xmlCleanupParser(void)
{
        // if this gets called, the libgcal we are using is broken
        exit(1);
}
			]],
			[[
const char *xml = "<nothing></nothing>";
char *dest;
get_the_url(xml, strlen(xml), &dest);
			]]
			)
		],
		[LG_TRY_AGAIN=yes],
		[AC_MSG_ERROR(
[
Your copy of libgcal is older than 0.9.6, and it contains
this bug:

http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=597153

You will need to upgrade to avoid segfaults when syncing with
Google calendar.
]
		)],
		[AC_MSG_ERROR(
[
Your copy of libgcal is older than 0.9.6.  Since you are
cross-compiling it is not possible to easily check whether
the following bug exists in your copy or not:

http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=597153

Please upgrade and try again.
]
		)]
		)
	LDFLAGS="$LGTEMP_LDFLAGS"
	if test "$LG_TRY_AGAIN" = "yes" ; then
		AC_MSG_RESULT([found patched libgcal...])
		PKG_CHECK_MODULES([LIBGCAL], [libgcal >= 0.9.4])
	fi
	]
	)

AC_LANG([C++])

#
# Search for a GUI-style version of su.  If not specified, we search for
# gksu (for Debian), beesu (for Fedora), and kdesu (for openSUSE),
# searched in the above order.
#
# User can override with the option.
#
AC_ARG_WITH([guisu],
	[AS_HELP_STRING([--with-guisu=CMD],
	[specify GUI oriented su wrapper program @<:@default: searches for gksu, beesu, or kdesu in that order@:>@])],
	[
		GUISU="$with_guisu"
		AC_MSG_NOTICE([Using user-specified GUI su wrapper: $GUISU])
	],
	[
		if test -x /usr/bin/gksu ; then
			GUISU="gksu"
			AC_MSG_NOTICE([Found GUI su wrapper: gksu])
		elif test -x /usr/bin/beesu ; then
			GUISU="beesu"
			AC_MSG_NOTICE([Found GUI su wrapper: beesu])
		elif test -x /usr/bin/kdesu ; then
			GUISU="kdesu"
			AC_MSG_NOTICE([Found GUI su wrapper: kdesu])
		else
			AC_MSG_ERROR([Unable to find a GUI oriented su wrapper.

	This is required for modem operation, in case permissions are not
	set on pppd properly.

	Please specify the GUI su wrapper you wish to use with the
	--with-guisu configure option.  Note that it MUST be a GUI.
	You cannot use the command line su.

	])
		fi
	])

AC_SUBST([GUISU])

#
# Check for Evolution data server libraries, and allow user to disable
# if needed.
#
AC_ARG_WITH([evolution],
	[AS_HELP_STRING([--with-evolution],
	[use evolution libs to help configure sync (yes,no,check) @<:@default=check@:>@])],
	[],
	[with_evolution=check])

if test "x$with_evolution" != xno ; then
	PKG_CHECK_MODULES([EVOLUTION],
		[libebook-1.2 libecal-1.2 libedataserver-1.2],
		[
			AC_DEFINE([HAVE_EVOLUTION], [1],
				[Define to 1 if Evolution data server libraries are available.])
		],
		[
			if test "x$with_evolution" != xcheck; then
				AC_MSG_FAILURE([--with-evolution was given, but test for evolution failed])
			fi
		])
fi

PKG_CHECK_MODULES([BARRY], [libbarry-19 libbarrysync-19 libbarrybackup-19])
PKG_CHECK_MODULES([GLIB2], [glib-2.0])
PKG_CHECK_MODULES([LIBXMLPP], [libxml++-2.6])
PKG_CHECK_MODULES([OPENSYNC22], [opensync-1.0], [], [OS22NOTFOUND=yes])
PKG_CHECK_MODULES([OPENSYNC40], [libopensync1], [], [OS40NOTFOUND=yes])

if test "$OS22NOTFOUND" = "yes" -a "$OS40NOTFOUND" = "yes" ; then
	AC_MSG_WARN(dnl
[

*************************************************************************
WARNING:
Unable to find development libraries for either opensync 0.22 or 0.4x.
Barry Desktop will be built without any sync support.
*************************************************************************

If this is not desired, consider adjusting the PKG_CONFIG_PATH environment
variable if you installed software in a non-standard prefix.

Alternatively, you may set the environment variables:

	OPENSYNC22_CFLAGS and OPENSYNC22_LIBS
or
	OPENSYNC40_CFLAGS and OPENSYNC40_LIBS

to avoid the need to call pkg-config.

See the pkg-config man page for more details.
])
fi

AM_CONDITIONAL([WITH_OPENSYNC22], [test "$OS22NOTFOUND" != "yes"])
AM_CONDITIONAL([WITH_OPENSYNC40], [test "$OS40NOTFOUND" != "yes"])

# Carry the special tree build environment variables from parent configure,
# just in case user is doing a complete tree build with --enable-desktop
AC_SUBST(TREE_BUILD_CXXFLAGS)
AC_SUBST(TREE_BUILD_LDFLAGS)

# Check for wxWidgets libraries
AM_OPTIONS_WXCONFIG
AM_PATH_WXCONFIG([2.8.0], [], [
	AC_MSG_ERROR([
	wxWidgets is required to build the Barry desktop,
	but the wx-config script could not be found.

	Please check that wx-config is in your path, that the directory
	where wxWidgets libraries are installed (returned by the
	'wx-config --libs' command) is in LD_LIBRARY_PATH, and that
	wxWidgets is version 2.3.4 or above.
	])
	])

# Setup system directories, so src/Makefile.am can use them for the compile
# and for the image/data file installations. (image_DATA)
basedatadir="$datadir/barry/desktop/"
imagedir="$datadir/barry/desktop/images/"
xmlmap22dir="$datadir/barry/desktop/0.22/"
xmlmap40dir="$datadir/barry/desktop/0.40/"
AC_SUBST(basedatadir)
AC_SUBST(imagedir)
AC_SUBST(xmlmap22dir)
AC_SUBST(xmlmap40dir)

# Checks for libraries.
AC_CHECK_LIB([pthread], [main])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([assert.h stdint.h time.h])

# Checks for typedefs, structures, and compiler characteristics.
#AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_CHECK_FUNCS([bzero gettimeofday memset select strcasecmp strchr strerror strtol strtoul])

AC_CONFIG_FILES([Makefile
                 po/Makefile.in
                 po-osyncwrap/Makefile.in
                 images/Makefile
                 src/Makefile
                 src/0.22/Makefile
                 src/0.40/Makefile
                 man/Makefile])
AC_OUTPUT


#
# Add a special hack at the end, to let the user disable RPATH if he wants.
#
# http://wiki.debian.org/RpathIssue
# http://lists.debian.org/debian-devel/2003/debian-devel-200306/msg00569.html
# http://fedoraproject.org/wiki/Packaging:Guidelines#Removing_Rpath
#
AC_ARG_ENABLE([rpathhack],
	[AC_HELP_STRING([--enable-rpathhack], [patch libtool to remove RPATH])],
	[
AC_MSG_RESULT([patching libtool to fix HIDEOUS BREAKAGE])
sed -i 's|^hardcode_libdir_flag_spec=.*|hardcode_libdir_flag_spec=""|g' libtool
sed -i 's|^runpath_var=LD_RUN_PATH|runpath_var=DIE_RPATH_DIE|g' libtool
	],
	[])

