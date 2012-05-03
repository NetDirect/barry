LIB_OSYNCWRAP_VERSION = @BARRY_DESKTOP_MAJOR@:@BARRY_DESKTOP_MINOR@:0

SUBDIRS = . 0.22 0.40

EXTRA_DIST = \
	ContactEditDlg.wxg \
	CalendarEditDlg.wxg \
	MemoEditDlg.wxg \
	TaskEditDlg.wxg \
	ModemDlg.wxg \
	oextract.cc

INCLUDES = $(TREE_BUILD_CXXFLAGS) $(BARRY_CFLAGS) $(OPENSYNC22_CFLAGS) $(OPENSYNC40_CFLAGS) $(GLIB2_CFLAGS) $(EVOLUTION_CFLAGS)
# opensync libraries are not included here... they are loaded with dlopen()
LIBS = $(TREE_BUILD_LDFLAGS) $(BARRY_LIBS) $(GLIB2_LIBS)

AM_CXXFLAGS = -ansi -Wall -g
AM_CPPFLAGS = -DBARRYDESKTOP_BASEDATADIR='"$(basedatadir)"' -DBARRYDESKTOP_IMAGEDIR='"$(imagedir)"' -DBARRYDESKTOP_PKGLIBEXECDIR='"$(pkglibexecdir)"' -DBARRYDESKTOP_SYSTEM_GUI_SU='"$(GUISU)"'
AM_LDFLAGS =

# This should really use @BARRY_MAJOR@ but that's from the root configure.ac
# not ours.  Desktop's major version should never really change from
# Barry's though, so we should be safe.
include_osyncwrapdir = ${includedir}/barry@BARRY_DESKTOP_MAJOR@/osyncwrap

noinst_PROGRAMS = ostest xmlcompactor xmlmap bsynccl
bin_PROGRAMS = barrydesktop
pkglibexec_PROGRAMS = bsyncjail blistevo
lib_LTLIBRARIES = libosyncwrap.la

include_osyncwrap_HEADERS = \
	error.h \
	dlopen.h \
	tempdir.h \
	ostypes.h \
	osbase.h osprivatebase.h \
	osconfig.h \
	osconv22.h \
	osconv40.h \
	os22.h \
	os40.h 

noinst_HEADERS =

libosyncwrap_la_SOURCES = \
	error.cc error.h \
	dlopen.cc dlopen.h \
	tempdir.cc tempdir.h \
	ostypes.cc ostypes.h \
	osbase.cc osbase.h osprivatebase.h \
	osconfig.cc osconfig.h \
	osconv22.h \
	osconv40.h \
	os22.h \
	os40.h 
if WITH_OPENSYNC22
libosyncwrap_la_SOURCES += os22.cc osconv22.cc
else
libosyncwrap_la_SOURCES += null-os22.cc
endif
if WITH_OPENSYNC40
libosyncwrap_la_SOURCES += os40.cc osconv40.cc
else
libosyncwrap_la_SOURCES += null-os40.cc
endif
libosyncwrap_la_LDFLAGS = -ldl -version-info ${LIB_OSYNCWRAP_VERSION}

barrydesktop_SOURCES = \
	barrydesktop.cc barrydesktop.h \
	ipc.h optout.h \
	EasyCondition.h \
	windowids.h \
	util.cc util.h \
	StringSync.cc StringSync.h \
	xmlcompactor.cc xmlcompactor.h \
	xmlmap.cc xmlmap.h \
	guitimet.cc guitimet.h \
	ClickImage.cc ClickImage.h \
	PNGButton.cc PNGButton.h \
	BaseButtons.cc BaseButtons.h \
	BaseFrame.cc BaseFrame.h \
	ContactPhotoWidget.cc ContactPhotoWidget.h \
	Mode.h \
	Mode_MainMenu.cc Mode_MainMenu.h \
	Mode_Sync.cc Mode_Sync.h \
	Mode_Browse.cc Mode_Browse.h \
	ModemDlg.cc ModemDlg.h \
	MigrateDlg.cc MigrateDlg.h \
	ContactEditDlg.cc ContactEditDlg.h \
	CalendarEditDlg.cc CalendarEditDlg.h \
		wxval.h \
	MemoEditDlg.cc MemoEditDlg.h \
	TaskEditDlg.cc TaskEditDlg.h \
	GroupCfgDlg.cc GroupCfgDlg.h \
	SyncStatusDlg.cc SyncStatusDlg.h \
	ConflictDlg.cc ConflictDlg.h \
	configui.cc configui.h \
	exechelper.cc exechelper.h \
	CUI_Barry.cc CUI_Barry.h \
	CUI_Evolution.cc CUI_Evolution.h \
	CUI_Google.cc CUI_Google.h \
	CUI_KDEPim.cc CUI_KDEPim.h \
	EvoSources.cc EvoSources.h \
	EvoCfgDlg.cc EvoCfgDlg.h \
	EvoDefaultDlg.cc EvoDefaultDlg.h \
	deviceset.cc deviceset.h
barrydesktop_LDADD = libosyncwrap.la -ldl @WX_LIBS@ $(LIBXMLPP_LIBS) $(LIBGCAL_LIBS) $(EVOLUTION_LIBS)
barrydesktop_CXXFLAGS = $(AM_CXXFLAGS) @WX_CXXFLAGS@ $(LIBXMLPP_CFLAGS) \
	$(LIBGCAL_CFLAGS)

bsyncjail_SOURCES = bsyncjail.cc ipc.h
bsyncjail_LDADD = libosyncwrap.la -ldl @WX_LIBS@
bsyncjail_CXXFLAGS = $(AM_CXXFLAGS) @WX_CXXFLAGS@

bsynccl_SOURCES = bsynccl.cc
bsynccl_LDADD = libosyncwrap.la -ldl
bsynccl_CXXFLAGS = $(AM_CXXFLAGS)

ostest_SOURCES = \
	deviceset.cc deviceset.h \
	ostest.cc
ostest_CXXFLAGS =
if WITH_OPENSYNC22
ostest_CXXFLAGS += -DWITH_OPENSYNC22
endif
if WITH_OPENSYNC40
ostest_CXXFLAGS += -DWITH_OPENSYNC40
endif
#ostest_LDFLAGS = -avoid-version
ostest_LDADD = libosyncwrap.la -ldl

xmlcompactor_SOURCES = xmlcompactor.cc xmlcompactor.h
xmlcompactor_CXXFLAGS = -DXMLCOMPACTOR $(AM_CXXFLAGS) $(LIBXMLPP_CFLAGS)
xmlcompactor_LDADD = $(LIBXMLPP_LIBS)

xmlmap_SOURCES = xmlmap.cc xmlmap.h
xmlmap_CXXFLAGS = -DXMLMAP $(AM_CXXFLAGS) $(LIBXMLPP_CFLAGS)
xmlmap_LDADD = $(LIBXMLPP_LIBS)

blistevo_SOURCES = blistevo.cc \
	EvoSources.cc EvoSources.h \
	dlopen.cc dlopen.h \
	error.cc error.h
blistevo_CXXFLAGS = $(AM_CXXFLAGS)
blistevo_LDADD = -ldl $(EVOLUTION_LIBS)

