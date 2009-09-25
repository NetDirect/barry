
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
	osbase.cc osbase.h \
	os22.h \
	os40.h \
	main.cc
if WITH_OPENSYNC22
barrydesktop_SOURCES += os22.cc
else
barrydesktop_SOURCES += null-os22.cc
endif
if WITH_OPENSYNC40
barrydesktop_SOURCES += os40.cc
else
barrydesktop_SOURCES += null-os40.cc
endif
# -rdynamic is recommended by the dlopen() manpage
barrydesktop_LDFLAGS = -rdynamic
#barrydesktop_LDFLAGS = -avoid-version
barrydesktop_LDADD = -ldl

noinst_HEADERS =

