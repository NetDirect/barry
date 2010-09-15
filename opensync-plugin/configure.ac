dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([Barry OpenSync Plugin], [0.17], [barry-devel@lists.sourceforge.net])
AC_CONFIG_SRCDIR(src/barry_sync.cc)
AC_CONFIG_HEADER(config.h)
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIR([../m4])
AM_INIT_AUTOMAKE(foreign)

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

AX_C_CHECK_FLAG([-fvisibility=hidden], [], [],
	[HAVE_C_GCCVISIBILITY=1],
	[HAVE_C_GCCVISIBILITY=0])
AX_CXX_CHECK_FLAG([-fvisibility=hidden], [], [],
	[HAVE_CXX_GCCVISIBILITY=1],
	[HAVE_CXX_GCCVISIBILITY=0])
AM_CONDITIONAL([WITH_GCCVISIBILITY], [test "$HAVE_C_GCCVISIBILITY" = "1" -a "$HAVE_CXX_GCCVISIBILITY" = "1"])

AC_LANG([C++])

PKG_CHECK_MODULES([BARRY], [libbarry-0 libbarrysync-0])

# Carry the special tree build environment variables from parent configure,
# just in case user is doing a complete tree build with --enable-opensync-plugin
AC_SUBST(TREE_BUILD_CXXFLAGS)
AC_SUBST(TREE_BUILD_LDFLAGS)

OPENSYNC_CONFIGDIR=$(pkg-config --variable=configdir opensync-1.0)
OPENSYNC_PLUGINDIR=$(pkg-config --variable=plugindir opensync-1.0)
OPENSYNC_FORMATSDIR=$(pkg-config --variable=formatsdir opensync-1.0)
OPENSYNC_HEADERDIR=$(pkg-config --variable=headerdir opensync-1.0)

AC_SUBST(OPENSYNC_CONFIGDIR) ## This is the place where you can install the default configuration files of your plugin
AC_SUBST(OPENSYNC_PLUGINDIR) ## This is the dir where you plugin will be installed
AC_SUBST(OPENSYNC_FORMATSDIR) ## Here are format plugins installed (if any)
AC_SUBST(OPENSYNC_HEADERDIR) ## Here are the headers that a user interface may need (if any)

AC_OUTPUT([
Makefile
src/Makefile
])
