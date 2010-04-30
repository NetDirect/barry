#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# Parts taken from the pkg.m4 file from pkg-config
# Copyright 2004 Scott James Remnant <scott@netsplit.com>
#

AC_PREREQ(2.61)
AC_INIT([barrydesktop], [0.1], [barry-devel@lists.sourceforge.net])
#AM_CONFIG_HEADER(config.h)
AC_CONFIG_SRCDIR([src/barrydesktop.cc])
AC_CONFIG_HEADERS([config.h:config.h.in])
AC_CONFIG_AUX_DIR([..])
AC_CONFIG_MACRO_DIR([../m4])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

AC_LANG([C++])

PKG_CHECK_MODULES([BARRY], [libbarry-0])
PKG_CHECK_MODULES([GLIB2], [glib-2.0])
PKG_CHECK_MODULES([LIBXMLPP], [libxml++-2.6])
PKG_CHECK_MODULES([OPENSYNC22], [opensync-1.0], [], [OS22NOTFOUND=yes])
PKG_CHECK_MODULES([OPENSYNC40], [libopensync1], [], [OS40NOTFOUND=yes])

if test "$OS22NOTFOUND" = "yes" -a "$OS40NOTFOUND" = "yes" ; then
	AC_MSG_ERROR(dnl
[
Unable to find development libraries for either opensync 0.22 or 0.4x.

Consider adjusting the PKG_CONFIG_PATH environment variable if you
installed software in a non-standard prefix.

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
                 images/Makefile
                 src/Makefile
                 src/0.22/Makefile
                 src/0.40/Makefile
                 man/Makefile])
AC_OUTPUT

