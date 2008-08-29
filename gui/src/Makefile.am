
##DEFAULT_INCLUDES =
INCLUDES = $(TREE_BUILD_CXXFLAGS) $(BARRY_CFLAGS) $(GTKMM_CFLAGS) $(LIBTAR_CFLAGS) $(LIBZ_CFLAGS)
LIBS = $(TREE_BUILD_LDFLAGS) $(BARRY_LIBS) $(GTKMM_LIBS) $(LIBTAR_LIBS) $(LIBZ_LIBS)

AM_CXXFLAGS = -ansi -Wall -g
AM_CPPFLAGS = -DBARRYBACKUP_GLADEDIR='"$(gladedir)"'

dist_glade_DATA = \
	BackupWindow.glade \
	DeviceSelectDlg.glade \
	DatabaseSelectDlg.glade \
	PasswordDlg.glade \
	PromptDlg.glade \
	ConfigDlg.glade

bin_PROGRAMS = barrybackup

barrybackup_SOURCES = \
	main.cc \
	BackupWindow.cc \
	DeviceSelectDlg.cc \
	DatabaseSelectDlg.cc \
	PasswordDlg.cc \
	PromptDlg.cc \
	ConfigDlg.cc \
	ConfigFile.cc \
	DeviceIface.cc \
	tarfile.cc \
	tarfile-ops-nt.cc \
	util.cc

noinst_HEADERS= \
	BackupWindow.h \
	ConfigDlg.h \
	ConfigFile.h \
	DatabaseSelectDlg.h \
	DeviceIface.h \
	DeviceSelectDlg.h \
	PasswordDlg.h \
	PromptDlg.h \
	tarfile.h \
	util.h

EXTRA_DIST = barry-gui.glade

