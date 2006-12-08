#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([barry], [0.4], [barry-devel@lists.sourceforge.net])
#AM_CONFIG_HEADER(config.h)
AC_CONFIG_SRCDIR([src/barry.h])
AC_CONFIG_HEADERS([config.h:config.h.in])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

AC_LANG([C++])

AC_ARG_WITH(libusb,
	[  --with-libusb=<path>    root path of libusb installation],
	[LIBUSB_LIB_PATH="$with_libusb/lib"
	 LIBUSB_INC_PATH="$with_libusb/include"],
	[LIBUSB_LIB_PATH="/usr/lib"
	 LIBUSB_INC_PATH="/usr/include"])
AC_ARG_WITH(boost,
	[  --with-boost=<path>     root path of boost installation (default=no boost)],
	[BOOST_LIB_PATH="$with_boost/lib"
	 BOOST_INC_PATH="$with_boost/include"
	 BOOST_ENABLED="true"],
	[BOOST_LIB_PATH="/usr/lib"
	 BOOST_INC_PATH="/usr/include"])

AC_SUBST(LIBUSB_LIB_PATH)
AC_SUBST(LIBUSB_INC_PATH)
AC_SUBST(BOOST_LIB_PATH)
AC_SUBST(BOOST_INC_PATH)

AM_CONDITIONAL([WITH_BOOST], [test "$BOOST_ENABLED" = "true"])

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
                 src/Makefile
                 tools/Makefile
                 examples/Makefile])
AC_OUTPUT

