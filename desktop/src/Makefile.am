
INCLUDES = $(TREE_BUILD_CXXFLAGS) $(BARRY_CFLAGS) $(OPENSYNC22_CFLAGS) $(OPENSYNC40_CFLAGS) $(GLIB2_CFLAGS)
# opensync libraries are not included here... they are loaded with dlopen()
LIBS = $(TREE_BUILD_LDFLAGS) $(BARRY_LIBS) $(GLIB2_LIBS)

AM_CXXFLAGS = -ansi -Wall -g
AM_CPPFLAGS =
AM_LDFLAGS =

bin_PROGRAMS = barrydesktop ostest

barrydesktop_SOURCES = \
	barrydesktop.cc
barrydesktop_LDADD = -ldl

ostest_SOURCES = \
	error.cc error.h \
	dlopen.cc dlopen.h \
	tempdir.cc tempdir.h \
	osbase.cc osbase.h \
	os22.h \
	os40.h \
	ostest.cc
if WITH_OPENSYNC22
ostest_SOURCES += os22.cc
else
ostest_SOURCES += null-os22.cc
endif
if WITH_OPENSYNC40
ostest_SOURCES += os40.cc
else
ostest_SOURCES += null-os40.cc
endif
#ostest_LDFLAGS = -avoid-version
ostest_LDADD = -ldl

noinst_HEADERS =

