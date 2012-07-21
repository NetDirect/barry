dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([Barry OpenSync Plugin 0.4x], [0.19.0], [barry-devel@lists.sourceforge.net])
AC_CONFIG_SRCDIR(src/barry_sync.cc)
AC_CONFIG_HEADER(config.h)
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE(foreign)

AM_GNU_GETTEXT([external])
# this is the verion of gettext, not barry
AM_GNU_GETTEXT_VERSION([0.18.1])

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

PKG_CHECK_MODULES([GLIB2], [glib-2.0])
PKG_CHECK_MODULES([OPENSYNC4X], [libopensync1])
PKG_CHECK_MODULES([BARRY], [libbarry-19])
PKG_CHECK_MODULES([BARRYSYNC], [libbarrysync-19])

# Carry the special tree build environment variables from parent configure,
# just in case user is doing a complete tree build with --enable-opensync-plugin
AC_SUBST(TREE_BUILD_CXXFLAGS)
AC_SUBST(TREE_BUILD_LDFLAGS)

OPENSYNC_CONFIGDIR=$(pkg-config --variable=configdir libopensync1)
OPENSYNC_PLUGINDIR=$(pkg-config --variable=plugindir libopensync1)
OPENSYNC_FORMATSDIR=$(pkg-config --variable=formatsdir libopensync1)
OPENSYNC_HEADERDIR=$(pkg-config --variable=headerdir libopensync1)

AC_SUBST(OPENSYNC_CONFIGDIR) ## This is the place where you can install the default configuration files of your plugin
AC_SUBST(OPENSYNC_PLUGINDIR) ## This is the dir where you plugin will be installed
AC_SUBST(OPENSYNC_FORMATSDIR) ## Here are format plugins installed (if any)
AC_SUBST(OPENSYNC_HEADERDIR) ## Here are the headers that a user interface may need (if any)

AC_OUTPUT([
Makefile
po/Makefile.in
src/Makefile
])

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

