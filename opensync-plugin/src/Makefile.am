barry_syncdir=@OPENSYNC_PLUGINDIR@
configdir=@OPENSYNC_CONFIGDIR@
#formatsdir=@OPENSYNC_FORMATSDIR@
opensyncheaderdir=@OPENSYNC_HEADERDIR@

INCLUDES = @TREE_BUILD_CXXFLAGS@ @GLIB2_CFLAGS@ @OPENSYNC2X_CFLAGS@ @BARRY_CFLAGS@ @BARRYSYNC_CFLAGS@
LIBS = @TREE_BUILD_LDFLAGS@ @GLIB2_LIBS@ @OPENSYNC2X_LIBS@ @BARRY_LIBS@ @BARRYSYNC_LIBS@

AM_CFLAGS = -Wall -Werror
AM_CXXFLAGS = -Wall -Werror
AM_LDFLAGS =
if WITH_GCCVISIBILITY
AM_CFLAGS += -D__BARRY_HAVE_GCCVISIBILITY__ -fvisibility=hidden
#AM_CXXFLAGS += -D__BARRY_HAVE_GCCVISIBILITY__ -fvisibility=hidden -fvisibility-inlines-hidden
AM_CXXFLAGS += -D__BARRY_HAVE_GCCVISIBILITY__ -fvisibility=hidden
#AM_LDFLAGS += -fvisibility=hidden -fvisibility-inlines-hidden
AM_LDFLAGS += -fvisibility=hidden
endif

EXTRA_DIST = barry_sync.h

dist_config_DATA = barry-sync

opensyncheader_HEADER = barry_sync.h

barry_sync_LTLIBRARIES = barry_sync.la

barry_sync_la_SOURCES = \
	barry_sync.cc barry_sync.h \
	environment.cc environment.h \
	vevent.cc vevent.h \
	vcard.cc vcard.h \
	idmap.cc idmap.h \
	trace.h

barry_sync_la_LDFLAGS = -avoid-version -export-dynamic -module
barry_sync_la_LIBADD = $(LIBS)

## if you plan to install format libraries: ##
#formats_LTLIBRARIES = format.la
#
#format_la_SOURCES = format.c
#format_la_LDFLAGS = -avoid-version -export-dynamic -module
#format_la_LIBADD = $(LIBS)

