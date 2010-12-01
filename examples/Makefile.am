
#DEFAULT_INCLUDES =
INCLUDES = $(LIBUSB_CFLAGS)
AM_CXXFLAGS = -Wall -g

noinst_PROGRAMS = \
	addcontact addcalendar addmemo \
	dbdump pipedump \
	usbclient

addcontact_SOURCES = addcontact.cc
addcontact_LDADD = ../src/libbarry.la $(LIBUSB_LIBS) -lpthread

addcalendar_SOURCES = addcalendar.cc
addcalendar_LDADD = ../src/libbarry.la $(LIBUSB_LIBS) -lpthread

addmemo_SOURCES = addmemo.cc
addmemo_LDADD = ../src/libbarry.la $(LIBUSB_LIBS) -lpthread

dbdump_SOURCES = dbdump.cc
dbdump_LDADD = ../src/libbarry.la $(LIBUSB_LIBS) -lpthread

pipedump_SOURCES = pipedump.cc
pipedump_LDADD = ../src/libbarry.la $(LIBUSB_LIBS) -lpthread

usbclient_SOURCES = usbclient.cc
usbclient_LDADD = ../src/libbarry.la $(LIBUSB_LIBS) -lpthread
