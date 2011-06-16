
#DEFAULT_INCLUDES =
INCLUDES =
AM_CXXFLAGS = -Wall -g

noinst_PROGRAMS = \
	addcontact addcalendar addmemo \
	dbdump pipedump \
	usbclient

addcontact_SOURCES = addcontact.cc
addcontact_LDADD = ../src/libbarry.la -lpthread

addcalendar_SOURCES = addcalendar.cc
addcalendar_LDADD = ../src/libbarry.la -lpthread

addmemo_SOURCES = addmemo.cc
addmemo_LDADD = ../src/libbarry.la -lpthread

dbdump_SOURCES = dbdump.cc
dbdump_LDADD = ../src/libbarry.la -lpthread

pipedump_SOURCES = pipedump.cc
pipedump_LDADD = ../src/libbarry.la -lpthread

usbclient_SOURCES = usbclient.cc
usbclient_LDADD = ../src/libbarry.la -lpthread
