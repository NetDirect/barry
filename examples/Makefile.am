
#DEFAULT_INCLUDES =
INCLUDES =
AM_CXXFLAGS = -Wall -g

noinst_PROGRAMS = \
	addcontact addcalendar addmemo \
	dbdump pipedump \
	usbclient

addcontact_SOURCES = addcontact.cc
addcontact_LDADD = ../src/libbarry.la $(THREAD_LIBRARY_LIBS)

addcalendar_SOURCES = addcalendar.cc
addcalendar_LDADD = ../src/libbarry.la $(THREAD_LIBRARY_LIBS)

addmemo_SOURCES = addmemo.cc
addmemo_LDADD = ../src/libbarry.la $(THREAD_LIBRARY_LIBS)

dbdump_SOURCES = dbdump.cc
dbdump_LDADD = ../src/libbarry.la $(THREAD_LIBRARY_LIBS)

pipedump_SOURCES = pipedump.cc
pipedump_LDADD = ../src/libbarry.la $(THREAD_LIBRARY_LIBS)

usbclient_SOURCES = usbclient.cc
usbclient_LDADD = ../src/libbarry.la $(THREAD_LIBRARY_LIBS)
