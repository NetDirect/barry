barry_syncdir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
#formatsdir=@OPENSYNC_FORMATSDIR@
opensyncheaderdir=@OPENSYNC_HEADERDIR@

INCLUDES = @PACKAGE_CFLAGS@ -I@BARRY_INC_PATH@ -I@LIBUSB_INC_PATH@

AM_CFLAGS = -Wall -Werror @PACKAGE_CFLAGS@

EXTRA_DIST = barry-config barry_sync.h

config_DATA = barry-config

opensyncheader_HEADER = barry_sync.h

barry_sync_LTLIBRARIES = barry_sync.la

barry_sync_la_SOURCES = barry_sync.cc
barry_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
barry_sync_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ @BARRY_LIB_PATH@/libbarry.a @LIBUSB_LIB_PATH@/libusb.a

## if you plan to install format libraries: ##
#formats_LTLIBRARIES = format.la
#
#format_la_SOURCES = format.c
#format_la_LDFLAGS = -avoid-version -export-dynamic -module
#format_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

