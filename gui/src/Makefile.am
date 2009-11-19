
##DEFAULT_INCLUDES =
INCLUDES = $(TREE_BUILD_CXXFLAGS) $(BARRY_CFLAGS) $(GTKMM_CFLAGS) $(LIBTAR_CFLAGS) $(LIBZ_CFLAGS)
LIBS = $(TREE_BUILD_LDFLAGS) $(BARRY_LIBS) $(GTKMM_LIBS) $(LIBTAR_LIBS) $(LIBZ_LIBS)

AM_CXXFLAGS = -ansi -Wall -g
AM_CPPFLAGS = -DBARRYBACKUP_GLADEDIR='"$(gladedir)"'

# To use gettext
datadir = @datadir@
localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

dist_glade_DATA = \
	BackupWindow.glade \
	DatabaseSelectDlg.glade \
	PasswordDlg.glade \
	PromptDlg.glade \
	ConfigDlg.glade

bin_PROGRAMS = barrybackup

barrybackup_SOURCES = \
	main.cc \
	BackupWindow.cc \
	DatabaseSelectDlg.cc \
	PasswordDlg.cc \
	PromptDlg.cc \
	ConfigDlg.cc \
	ConfigFile.cc \
	DeviceBus.cc \
	DeviceIface.cc \
	Thread.cc \
	Pin.cc \
	tarfile.cc \
	tarfile-ops-nt.cc \
	util.cc

noinst_HEADERS= \
	BackupWindow.h \
	ConfigDlg.h \
	ConfigFile.h \
	DatabaseSelectDlg.h \
	DeviceBus.h \
	DeviceIface.h \
	PasswordDlg.h \
	PromptDlg.h \
	Thread.h \
	Pin.h \
	tarfile.h \
	util.h

EXTRA_DIST = barry-gui.glade

