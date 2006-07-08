dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.58)
AC_INIT([Barry OpenSync Plugin], 0.01, [], barryplugin)
AM_INIT_AUTOMAKE(foreign)
AC_CONFIG_SRCDIR(src/barry_sync.cc)
AC_CONFIG_HEADER(config.h)

export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/lib/pkgconfig:$prefix/lib/pkgconfig:/usr/local/lib/pkgconfig
pkg_modules="opensync-1.0 glib-2.0"
PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_CXX
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

AC_LANG([C++])

AC_ARG_WITH(barry,
	[  --with-barry=<path>     root path of Barry installation],
	[BARRY_LIB_PATH="$with_barry/lib"
	 BARRY_INC_PATH="$with_barry/include"],
	[BARRY_LIB_PATH="../../src/install/lib"
	 BARRY_INC_PATH="../../src/install/include"])

AC_ARG_WITH(libusb,
	[  --with-libusb=<path>    root path of libusb installation],
	[LIBUSB_LIB_PATH="$with_libusb/lib"
	 LIBUSB_INC_PATH="$with_libusb/include"],
	[LIBUSB_LIB_PATH="/usr/lib"
	 LIBUSB_INC_PATH="/usr/include"])

OPENSYNC_CONFIGDIR=$(pkg-config --variable=configdir opensync-1.0)
OPENSYNC_PLUGINDIR=$(pkg-config --variable=plugindir opensync-1.0)
OPENSYNC_FORMATSDIR=$(pkg-config --variable=formatsdir opensync-1.0)
OPENSYNC_HEADERDIR=$(pkg-config --variable=headerdir opensync-1.0)

AC_SUBST(OPENSYNC_CONFIGDIR) ## This is the place where you can install the default configuration files of your plugin
AC_SUBST(OPENSYNC_PLUGINDIR) ## This is the dir where you plugin will be installed
AC_SUBST(OPENSYNC_FORMATSDIR) ## Here are format plugins installed (if any)
AC_SUBST(OPENSYNC_HEADERDIR) ## Here are the headers that a user interface may need (if any)
AC_SUBST(BARRY_LIB_PATH)
AC_SUBST(BARRY_INC_PATH)
AC_SUBST(LIBUSB_LIB_PATH)
AC_SUBST(LIBUSB_INC_PATH)

AC_OUTPUT([
Makefile
src/Makefile
])
