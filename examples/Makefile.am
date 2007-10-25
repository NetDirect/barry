
#DEFAULT_INCLUDES =
INCLUDES = $(LIBUSB_CFLAGS)
AM_CXXFLAGS = -ansi -Wall -g

noinst_PROGRAMS = addcontact addcalendar

addcontact_SOURCES = addcontact.cc
addcontact_LDADD = ../src/libbarry.la $(LIBUSB_LIBS)

addcalendar_SOURCES = addcalendar.cc
addcalendar_LDADD = ../src/libbarry.la $(LIBUSB_LIBS)

