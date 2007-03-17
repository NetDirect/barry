
##DEFAULT_INCLUDES =
INCLUDES = $(BARRY_CFLAGS) $(GTKMM_CFLAGS) $(LIBTAR_CFLAGS) $(LIBZ_CFLAGS)
LIBS = $(BARRY_LIBS) $(GTKMM_LIBS) $(LIBTAR_LIBS) $(LIBZ_LIBS)

AM_CXXFLAGS = -ansi -Wall -g
AM_CPPFLAGS = -DBARRYBACKUP_GLADEDIR='"$(gladedir)"'

glade_DATA = \
	BackupWindow.glade \
	DeviceSelectDlg.glade \
	DatabaseSelectDlg.glade \
	PasswordDlg.glade \
	ConfigDlg.glade

EXTRA_DIST = \
	$(glade_DATA)

bin_PROGRAMS = barrybackup

barrybackup_SOURCES = \
	main.cc \
	aboutdialog.cc \
	BackupWindow.cc \
	DeviceSelectDlg.cc \
	DatabaseSelectDlg.cc \
	PasswordDlg.cc \
	ConfigDlg.cc \
	ConfigFile.cc \
	DeviceIface.cc \
	tarfile.cc \
	tarfile-ops-nt.cc \
	util.cc

