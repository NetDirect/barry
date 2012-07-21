#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([barry-backup], [0.19.0], [barry-devel@lists.sourceforge.net])
#AM_CONFIG_HEADER(config.h)
AC_CONFIG_SRCDIR([src/main.cc])
AC_CONFIG_HEADERS([config.h:config.h.in])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

AM_GNU_GETTEXT([external])
# this is the gettext version, not barry's version
AM_GNU_GETTEXT_VERSION([0.18.1])

#
# Checks for programs.
#
AC_PROG_CC
AC_PROG_CXX
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

AC_LANG([C++])

PKG_CHECK_MODULES([BARRY], [libbarry-19])
PKG_CHECK_MODULES([BARRYBACKUP], [libbarrybackup-19])
PKG_CHECK_MODULES([GTKMM], [gtkmm-2.4 libglademm-2.4 gthread-2.0])

# Carry the special tree build environment variables from parent configure,
# just in case user is doing a complete tree build with --enable-gui
AC_SUBST(TREE_BUILD_CXXFLAGS)
AC_SUBST(TREE_BUILD_LDFLAGS)

# Setup gladedir, so src/Makefile.am can use it for the compile
# and for the glade file installations. (glade_DATA)
gladedir="$datadir/barry/glade/"
AC_SUBST(gladedir)

# Checks for libraries.
#AC_CHECK_LIB([IOKit], [main])
AC_CHECK_LIB([pthread], [main])
#AC_CHECK_LIB([libusb], [libusb_init])

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
                 src/Makefile
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

