#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([barry], [0.15], [barry-devel@lists.sourceforge.net])
#AM_CONFIG_HEADER(config.h)
AC_CONFIG_SRCDIR([src/barry.h])
AC_CONFIG_HEADERS([config.h:config.h.in])
AM_INIT_AUTOMAKE([dist-bzip2])

AC_USE_SYSTEM_EXTENSIONS

#
# Checks for programs.
#
AC_PROG_CC
AC_PROG_CXX
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

AX_C_CHECK_FLAG([-fvisibility=hidden], [], [],
	[HAVE_C_GCCVISIBILITY=1],
	[HAVE_C_GCCVISIBILITY=0])
AX_CXX_CHECK_FLAG([-fvisibility=hidden], [], [],
	[HAVE_CXX_GCCVISIBILITY=1],
	[HAVE_CXX_GCCVISIBILITY=0])
AM_CONDITIONAL([WITH_GCCVISIBILITY], [test "$HAVE_C_GCCVISIBILITY" = "1" -a "$HAVE_CXX_GCCVISIBILITY" = "1"])

AC_LANG([C++])

#
# Checks for libraries.
#

# generates LIBUSB_CFLAGS and LIBUSB_LIBS for us
PKG_CHECK_MODULES([LIBUSB], [libusb],
	[],
	[echo "ERROR: Libusb not found automatically... build may fail if you don't specify --with-libusb";LIBUSB_CFLAGS="-I/usr/include" LIBUSB_LIBS="-lusb"])

AC_ARG_WITH(libusb,
	[  --with-libusb=<path>    root path of libusb installation],
	[LIBUSB_CFLAGS="-I$with_libusb/include"
	 LIBUSB_LIBS="-L$with_libusb/lib -lusb"],
	[])

AC_ARG_WITH(boost,
	AC_HELP_STRING(--with-boost[=path],
		[root path of boost installation (no arg defaults to 'yes' and uses system paths)]),
	use_boost=$withval,  use_boost=no ) 

if test x"$use_boost" != "xyes" ; then
	if test x"$use_boost" != "xno" ; then
		BOOST_LIB_PATH=$use_boost/lib
		BOOST_INC_PATH=$use_boost/include
		BOOST_ENABLED="true"
	fi
else
	BOOST_LIB_PATH=""
	BOOST_INC_PATH=""
	BOOST_ENABLED="true"	
fi

#PKG_CHECK_MODULES([OPENSSL], [openssl])

PKG_CHECK_MODULES([FUSE], [fuse >= 2.5],
	[FUSE_FOUND=1],
	[echo "FUSE library not found, skipping fuse module."; FUSE_FOUND=0]
	)

pkgconfigdir=${libdir}/pkgconfig
AC_SUBST(pkgconfigdir)

AC_SUBST(BOOST_LIB_PATH)
AC_SUBST(BOOST_INC_PATH)
AC_SUBST(LIBUSB_CFLAGS)
AC_SUBST(LIBUSB_LIBS)

AM_CONDITIONAL([WITH_BOOST], [test "$BOOST_ENABLED" = "true"])
AM_CONDITIONAL([WITH_BOOST_PATHS], [test -n "$BOOST_LIB_PATH"])
AM_CONDITIONAL([WITH_FUSE], [test "$FUSE_FOUND" = "1"])

#AC_CHECK_LIB([IOKit], [main])
AC_CHECK_LIB([pthread], [main])
#AC_CHECK_LIB([libusb], [libusb_init])

AM_ICONV


#
# Checks for header files.
#
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([assert.h stdint.h time.h])

#
# Checks for typedefs, structures, and compiler characteristics.
#
#AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

#
# Checks for library functions.
#

# checks that are buggy and need a C compiler only
AC_LANG([C])
# AC_FUNC_STRNLEN changes linker options for us, and depends on a src/strnlen.c
AC_FUNC_STRNLEN
if test $ac_cv_func_strnlen_working = yes ; then
	AC_DEFINE([HAVE_WORKING_STRNLEN], 1,
		[Define to 1 if a working strnlen exists.])
fi
if test $ac_cv_func_strnlen_working = no ; then
	AC_DEFINE([HAVE_WORKING_STRNLEN], 0,
		[Define to 1 if a working strnlen exists, 0 if not.])
fi

# checks that work with C++
AC_LANG([C++])
AC_FUNC_CLOSEDIR_VOID
AC_PROG_GCC_TRADITIONAL
#AC_FUNC_MALLOC
#AC_FUNC_MKTIME
#AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
#AC_FUNC_STAT

AC_CHECK_FUNCS([bzero gettimeofday memset select strcasecmp strchr strerror strtol strtoul])
AC_C_BIGENDIAN

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 tools/Makefile
                 examples/Makefile
                 man/Makefile
                 libbarry-0.pc])

#
# nested packages
#
AC_ARG_ENABLE([gui], [AC_HELP_STRING([--enable-gui], [build the gui])])
if test "$enable_gui" = yes; then
	AC_CONFIG_SUBDIRS([gui])
fi
AC_ARG_ENABLE([opensync-plugin], [AC_HELP_STRING([--enable-opensync-plugin], [build the opensync plugin])])
if test "$enable_opensync_plugin" = yes; then
	AC_CONFIG_SUBDIRS([opensync-plugin])
fi
if test "$enable_gui" = yes || test "$enable_opensync_plugin" = yes; then
	export TREE_BUILD_CXXFLAGS="-I`pwd`"
	export TREE_BUILD_LDFLAGS="-L`pwd`/src"
	export PKG_CONFIG_PATH="`pwd`:$PKG_CONFIG_PATH"
fi

AC_OUTPUT

