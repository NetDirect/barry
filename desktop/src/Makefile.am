
INCLUDES = $(TREE_BUILD_CXXFLAGS) $(BARRY_CFLAGS) $(OPENSYNC22_CFLAGS) $(OPENSYNC40_CFLAGS) $(GLIB2_CFLAGS)
# opensync libraries are not included here... they are loaded with dlopen()
LIBS = $(TREE_BUILD_LDFLAGS) $(BARRY_LIBS) $(GLIB2_LIBS)

AM_CXXFLAGS = -ansi -Wall -g
AM_CPPFLAGS =
AM_LDFLAGS =

bin_PROGRAMS = barrydesktop

barrydesktop_SOURCES = \
	error.cc error.h \
	dlopen.cc dlopen.h \
	os22.cc os22.h \
	os40.cc os40.h \
	osbase.h \
	main.cc
# -rdynamic is recommended by the dlopen() manpage
barrydesktop_LDFLAGS = -rdynamic
#barrydesktop_LDFLAGS = -avoid-version
barrydesktop_LDADD = -ldl

noinst_HEADERS =

