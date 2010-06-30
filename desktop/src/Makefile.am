
SUBDIRS = . 0.22 0.40

INCLUDES = $(TREE_BUILD_CXXFLAGS) $(BARRY_CFLAGS) $(OPENSYNC22_CFLAGS) $(OPENSYNC40_CFLAGS) $(GLIB2_CFLAGS)
# opensync libraries are not included here... they are loaded with dlopen()
LIBS = $(TREE_BUILD_LDFLAGS) $(BARRY_LIBS) $(GLIB2_LIBS)

AM_CXXFLAGS = -ansi -Wall -g
AM_CPPFLAGS = -DBARRYDESKTOP_BASEDATADIR='"$(basedatadir)"' -DBARRYDESKTOP_IMAGEDIR='"$(imagedir)"'
AM_LDFLAGS =

bin_PROGRAMS = barrydesktop bsyncjail bsynccl
noinst_PROGRAMS = ostest xmlcompactor xmlmap

barrydesktop_SOURCES = \
	barrydesktop.cc barrydesktop.h \
	windowids.h \
	util.cc util.h \
	xmlcompactor.cc xmlcompactor.h \
	xmlmap.cc xmlmap.h \
	tzwrapper.cc tzwrapper.h \
	ClickImage.cc ClickImage.h \
	PNGButton.cc PNGButton.h \
	BaseButtons.cc BaseButtons.h \
	BaseFrame.cc BaseFrame.h \
	Mode.h \
	Mode_MainMenu.cc Mode_MainMenu.h \
	Mode_Sync.cc Mode_Sync.h \
	GroupCfgDlg.cc GroupCfgDlg.h \
	SyncStatusDlg.cc SyncStatusDlg.h \
	ConflictDlg.cc ConflictDlg.h \
	configui.cc configui.h \
	exechelper.cc exechelper.h \
	CUI_Barry.cc CUI_Barry.h \
	CUI_Evolution.cc CUI_Evolution.h \
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
barrydesktop_LDADD = -ldl @WX_LIBS@ $(LIBXMLPP_LIBS)
barrydesktop_CXXFLAGS = $(AM_CXXFLAGS) @WX_CXXFLAGS@ $(LIBXMLPP_CFLAGS)

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

bsynccl_SOURCES = \
	bsynccl.cc \
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
bsynccl_SOURCES += os22.cc osconv22.cc
else
bsynccl_SOURCES += null-os22.cc
endif
if WITH_OPENSYNC40
bsynccl_SOURCES += os40.cc osconv40.cc
else
bsynccl_SOURCES += null-os40.cc
endif
bsynccl_LDADD = -ldl
bsynccl_CXXFLAGS = $(AM_CXXFLAGS)

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

xmlcompactor_SOURCES = xmlcompactor.cc xmlcompactor.h
xmlcompactor_CXXFLAGS = -DXMLCOMPACTOR $(AM_CXXFLAGS) $(LIBXMLPP_CFLAGS)
xmlcompactor_LDADD = $(LIBXMLPP_LIBS)

xmlmap_SOURCES = \
	tzwrapper.cc tzwrapper.h \
	xmlmap.cc xmlmap.h
xmlmap_CXXFLAGS = -DXMLMAP $(AM_CXXFLAGS) $(LIBXMLPP_CFLAGS)
xmlmap_LDADD = $(LIBXMLPP_LIBS)

noinst_HEADERS =

