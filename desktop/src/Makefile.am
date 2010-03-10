
INCLUDES = $(TREE_BUILD_CXXFLAGS) $(BARRY_CFLAGS) $(OPENSYNC22_CFLAGS) $(OPENSYNC40_CFLAGS) $(GLIB2_CFLAGS)
# opensync libraries are not included here... they are loaded with dlopen()
LIBS = $(TREE_BUILD_LDFLAGS) $(BARRY_LIBS) $(GLIB2_LIBS)

AM_CXXFLAGS = -ansi -Wall -g
AM_CPPFLAGS = -DBARRYDESKTOP_IMAGEDIR='"$(imagedir)"'
AM_LDFLAGS =

bin_PROGRAMS = barrydesktop bsyncjail
noinst_PROGRAMS = ostest

barrydesktop_SOURCES = \
	barrydesktop.cc \
	windowids.h \
	GroupCfgDlg.cc GroupCfgDlg.h \
	SyncStatusDlg.cc SyncStatusDlg.h \
	ConflictDlg.cc ConflictDlg.h \
	configui.h configui.cc \
	exechelper.h exechelper.cc \
	CUI_Evolution.h CUI_Evolution.cc \
	error.cc error.h \
	dlopen.cc dlopen.h \
	tempdir.cc tempdir.h \
	deviceset.cc deviceset.h \
	osbase.cc osbase.h osprivatebase.h \
	osconfig.cc osconfig.h \
	osconv22.h \
	osconv40.h \
	os22.h \
	os40.h 
if WITH_OPENSYNC22
barrydesktop_SOURCES += os22.cc osconv22.cc
else
barrydesktop_SOURCES += null-os22.cc
endif
if WITH_OPENSYNC40
barrydesktop_SOURCES += os40.cc osconv40.cc
else
barrydesktop_SOURCES += null-os40.cc
endif
barrydesktop_LDADD = -ldl @WX_LIBS@
barrydesktop_CXXFLAGS = $(AM_CXXFLAGS) @WX_CXXFLAGS@

bsyncjail_SOURCES = \
	bsyncjail.cc \
	error.cc error.h \
	dlopen.cc dlopen.h \
	tempdir.cc tempdir.h \
	osbase.cc osbase.h osprivatebase.h \
	osconfig.cc osconfig.h \
	osconv22.h \
	osconv40.h \
	os22.h \
	os40.h 
if WITH_OPENSYNC22
bsyncjail_SOURCES += os22.cc osconv22.cc
else
bsyncjail_SOURCES += null-os22.cc
endif
if WITH_OPENSYNC40
bsyncjail_SOURCES += os40.cc osconv40.cc
else
bsyncjail_SOURCES += null-os40.cc
endif
bsyncjail_LDADD = -ldl @WX_LIBS@
bsyncjail_CXXFLAGS = $(AM_CXXFLAGS) @WX_CXXFLAGS@

ostest_SOURCES = \
	error.cc error.h \
	dlopen.cc dlopen.h \
	tempdir.cc tempdir.h \
	deviceset.cc deviceset.h \
	osbase.cc osbase.h osprivatebase.h \
	osconfig.cc osconfig.h \
	osconv22.h \
	osconv40.h \
	os22.h \
	os40.h \
	ostest.cc
if WITH_OPENSYNC22
ostest_SOURCES += os22.cc osconv22.cc
else
ostest_SOURCES += null-os22.cc
endif
if WITH_OPENSYNC40
ostest_SOURCES += os40.cc osconv40.cc
else
ostest_SOURCES += null-os40.cc
endif
#ostest_LDFLAGS = -avoid-version
ostest_LDADD = -ldl

noinst_HEADERS =

