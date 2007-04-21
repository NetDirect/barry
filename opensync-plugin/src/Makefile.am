barry_syncdir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
#formatsdir=@OPENSYNC_FORMATSDIR@
opensyncheaderdir=@OPENSYNC_HEADERDIR@

INCLUDES = @PACKAGE_CFLAGS@ @BARRY_CFLAGS@

AM_CFLAGS = -Wall -Werror
AM_CXXFLAGS = -Wall -Werror

EXTRA_DIST = barry_sync.h

config_DATA = barry-sync

opensyncheader_HEADER = barry_sync.h

barry_sync_LTLIBRARIES = barry_sync.la

barry_sync_la_SOURCES = \
	barry_sync.cc barry_sync.h \
	environment.cc environment.h \
	vevent.cc vevent.h \
	vformat.c vformat.h \
	trace.h

barry_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
barry_sync_la_LIBADD = @PACKAGE_LIBS@ @LIBS@ @BARRY_LIBS@

## if you plan to install format libraries: ##
#formats_LTLIBRARIES = format.la
#
#format_la_SOURCES = format.c
#format_la_LDFLAGS = -avoid-version -export-dynamic -module
#format_la_LIBADD = @PACKAGE_LIBS@ @LIBS@

