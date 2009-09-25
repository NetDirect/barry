#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([barry], [0.18.0], [barry-devel@lists.sourceforge.net])
#AM_CONFIG_HEADER(config.h)
AC_CONFIG_SRCDIR([src/barry.h])
AC_CONFIG_HEADERS([config.h:config.h.in])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([dist-bzip2])

AC_USE_SYSTEM_EXTENSIONS

AM_GNU_GETTEXT([external])
# this is the verion of gettext, not barry
AM_GNU_GETTEXT_VERSION([0.17])

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

# libtar and libz don't have pkg-config files on many systems.
# libz we can assume, but libtar we need to test for
AC_ARG_WITH(libtar,
	[  --with-libtar=<path>    root path of libtar install],
	[LIBTAR_CFLAGS="-I$with_libtar/include"
	 LIBTAR_LIBS="-L$with_libtar/lib -ltar"],
	[echo "Guessing libtar location... may not compile...";
	 LIBTAR_CFLAGS=""
	 LIBTAR_LIBS="-ltar"])
AC_SUBST(LIBTAR_CFLAGS)
AC_SUBST(LIBTAR_LIBS)

AC_CHECK_LIB([tar], [tar_open],
	[LIBTAR_FOUND=1
	AC_MSG_NOTICE([Found libtar, enabling libbarrybackup]) ],
	[LIBTAR_FOUND=0
	AC_MSG_NOTICE([Libtar NOT found, disabling libbarrybackup])],
	[$LIBTAR_CFLAGS $LIBTAR_LIBS])

AC_ARG_WITH(libz,
	[  --with-zlib=<path>      root path of zlib install],
	[LIBZ_CFLAGS="-I$with_libtar/include"
	 LIBZ_LIBS="-L$with_libtar/lib -ltar"],
	[echo "Guessing zlib location... may not compile...";
	 LIBZ_CFLAGS=""
	 LIBZ_LIBS="-lz"])
AC_SUBST(LIBZ_CFLAGS)
AC_SUBST(LIBZ_LIBS)

AM_CONDITIONAL([WITH_BACKUP], [test "$LIBTAR_FOUND" = "1"])

# generates LIBUSB_CFLAGS and LIBUSB_LIBS for us
PKG_CHECK_MODULES([LIBUSB], [libusb],
	[],
	[echo "ERROR: Libusb not found automatically... build may fail if you don't specify --with-libusb";LIBUSB_CFLAGS="-I/usr/include" LIBUSB_LIBS="-lusb"])

AC_ARG_WITH(libusb,
	[  --with-libusb=<path>    root path of libusb installation],
	[LIBUSB_CFLAGS="-I$with_libusb/include"
	 LIBUSB_LIBS="-L$with_libusb/lib -lusb"],
	[])

#
# Boost library configuration
#
# Ok, the requirements:
#
#    - let the user enable/disable Boost support from configure
#         - default to disabled
#         - if enabled, and not available, error
#
#    - let user specify include path, and lib path, separately,
#      since some Boost installations have an additional boost-1.34.1
#      style directory in them
#         - default to using no path overrides, assuming everything
#           that is needed is in default distro locations
#
#    - let user specify the name of the serialization library, since
#      the name of the library can change whether you're building
#      from source or not
#         - default to searching for boost_serialization or
#           boost_serialization-mt, and error if not found
#
# Therefore:
#
#    --enable-boost                   Handles enable/disable
#    --with-boost-include=path        Override the include path
#    --with-boost-lib=path            Override the lib path
#    --with-boost-serialization=name  Override the name of serialization
#                                     library to link with
#

AC_ARG_ENABLE([boost],
	AC_HELP_STRING([--enable-boost], [enable Boost support]),
	[
	if test x"$enableval" = "xno" ; then
		BOOST_ENABLED=no
	else
		BOOST_ENABLED=yes
	fi
	],
	[BOOST_ENABLED=no])

AC_ARG_WITH([boost-include],
	AC_HELP_STRING(--with-boost-include=path,
		[path to Boost include directory in order to make include <boost/something.hpp> valid (defaults to system paths)]),
	BOOST_INC_PATH="-I$withval",  BOOST_INC_PATH="" )

AC_ARG_WITH([boost-lib],
	AC_HELP_STRING(--with-boost-lib=path,
		[path to Boost library directory (defaults to system paths)]),
	BOOST_LIB_PATH="-L$withval",  BOOST_LIB_PATH="" )

AC_ARG_WITH(boost-serialization,
	AC_HELP_STRING(--with-boost-serialization=name,
		[name of serialization library to use with compiler's -l option (defaults to boost_serialization or boost_serialization-mt.)]),
	boost_serialization_name=$withval, boost_serialization_name="boost_serialization")

AC_MSG_NOTICE([using BOOST library... $BOOST_ENABLED])
if test x"$BOOST_INC_PATH$BOOST_LIB_PATH" != x ; then
	AC_MSG_NOTICE([BOOST include options: $BOOST_INC_PATH])
	AC_MSG_NOTICE([BOOST library options: $BOOST_LIB_PATH])
fi

if test x"$BOOST_ENABLED" = "xyes" ; then
	# Only $BOOST_LIB_PATH is given for this check, since no
	# headers are included in the autoconf main() test snippet.
	AC_CHECK_LIB($boost_serialization_name, main,
		[BOOST_LDADD="-l$boost_serialization_name"],
		[
			AC_CHECK_LIB(boost_serialization-mt, main,
				[BOOST_LDADD="-lboost_serialization-mt"],
				[AC_MSG_ERROR(boost_serialization not found)],
				[$BOOST_LIB_PATH]
				)
		],
		[$BOOST_LIB_PATH])

	if test x"$BOOST_LDADD" != x ; then
		AC_MSG_NOTICE([BOOST library name: $BOOST_LDADD])
	else
		AC_MSG_ERROR([boost_serialization library not found])
	fi
fi

AC_SUBST(BOOST_LIB_PATH)
AC_SUBST(BOOST_INC_PATH)
AC_SUBST(BOOST_LDADD)
AM_CONDITIONAL([WITH_BOOST], [test "$BOOST_ENABLED" = "yes"])


#PKG_CHECK_MODULES([OPENSSL], [openssl])

PKG_CHECK_MODULES([FUSE], [fuse >= 2.5],
	[FUSE_FOUND=1],
	[echo "FUSE library not found, skipping fuse module."; FUSE_FOUND=0]
	)

PKG_CHECK_MODULES([GLIB2], [glib-2.0],
	[GLIB2_FOUND=1],
	[echo "GLIB 2.0 not found, skipping sync library."; GLIB2_FOUND=0]
	)

pkgconfigdir=${libdir}/pkgconfig
AC_SUBST(pkgconfigdir)

AC_SUBST(LIBUSB_CFLAGS)
AC_SUBST(LIBUSB_LIBS)

AM_CONDITIONAL([WITH_FUSE], [test "$FUSE_FOUND" = "1"])
AM_CONDITIONAL([WITH_SYNC], [test "$GLIB2_FOUND" = "1"])

#AC_CHECK_LIB([IOKit], [main])
#AC_CHECK_LIB([libusb], [libusb_init])
AC_CHECK_LIB([pthread], [main])

AC_ARG_WITH(zlib,
	AC_HELP_STRING(--with-zlib, [force usage of zlib, and halt if not available]),
	force_zlib=$withval, force_zlib=no )

AC_CHECK_LIB([z], [crc32],
	[
		AC_DEFINE([HAVE_ZLIB], [1], [Use crc32 when generating packed .cod files])
		AC_ARG_VAR([ZLIB_LIBS], [Linker options for zlib])
		ZLIB_LIBS="-lz"
	],
	[
		echo "*****************************************************************"
		echo "WARNING: zlib not found... packed .cod files will fail crc checks"
		echo "*****************************************************************"
		AC_ARG_VAR([ZLIB_LIBS], [Linker options for zlib])
		ZLIB_LIBS=""
		if test "x$force_zlib" != xno ; then
			AC_MSG_FAILURE([--with-zlib specified, but zlib not found])
		fi
	]
	)

PKG_CHECK_MODULES([LIBXMLXX], [libxml++-2.6],
	[LIBXMLXX_FOUND=1],
	[
		echo "*****************************************************************"
		echo "WARNING: libxml++ not found... ALX parser not included"
		echo "*****************************************************************"
		LIBXMLXX_FOUND=0
	]
	)

AM_CONDITIONAL([WITH_ALX],  [test "$LIBXMLXX_FOUND" = "1"])


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

AC_CONFIG_FILES([Makefile po/Makefile.in
                 src/Makefile
                 tools/Makefile
                 examples/Makefile
                 man/Makefile
                 libbarry-0.pc
		 libbarrydp-0.pc
		 libbarryjdwp-0.pc
		 libbarrysync-0.pc
		 libbarrybackup-0.pc
		 libbarryalx-0.pc])

#
# nested packages
#
AC_ARG_ENABLE([gui], [AC_HELP_STRING([--enable-gui], [build the gui])])
AC_ARG_ENABLE([opensync-plugin], [AC_HELP_STRING([--enable-opensync-plugin], [build the opensync plugin])])
AC_ARG_ENABLE([opensync-plugin-4x], [AC_HELP_STRING([--enable-opensync-plugin-4x], [build the opensync 0.4x plugin])])
AC_ARG_ENABLE([desktop], [AC_HELP_STRING([--enable-desktop], [build the desktop control panel])])

# make sure that the environment is setup before anything else
if test "$enable_gui" = yes || test "$enable_opensync_plugin" = yes || test "$enable_opensync_plugin_4x" = yes || test "$enable_desktop" = yes; then
	export TREE_BUILD_CXXFLAGS="-I`pwd`"
	export TREE_BUILD_LDFLAGS="-L`pwd`/src"
	export PKG_CONFIG_PATH="`pwd`:$PKG_CONFIG_PATH"
fi

# conditionally enable the subdirectory packages
if test "$enable_gui" = yes; then
	AC_CONFIG_SUBDIRS([gui])
fi
if test "$enable_opensync_plugin" = yes; then
	AC_CONFIG_SUBDIRS([opensync-plugin])
fi
if test "$enable_opensync_plugin_4x" = yes; then
	AC_CONFIG_SUBDIRS([opensync-plugin-0.4x])
fi
if test "$enable_desktop" = yes; then
	AC_CONFIG_SUBDIRS([desktop])
fi

#
# Generate the scripts!
#
AC_OUTPUT

