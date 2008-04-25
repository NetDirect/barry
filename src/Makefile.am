
# The following version numbers are:  current:revision:age
#
# See also: http://sources.redhat.com/autobook/autobook/autobook_91.html
#
# Current - number of the current interface of the library
# Revision - implementation number of most recent interface
# Age - number of previous additional interfaces supported by this library
#
# Libtool uses the following calculation for Linux:
#
#    major = current - age
#    suffix = major.age.revision
#
# Therefore a libtool version of 1.4.0 will translate to a filename of:
#    libbarry.so.1.0.4
#
# Other operating systems just use current.revision, most of the time,
# and age is only used to subtract from current.
#
# Therefore, if you are careful never to increment the major version
# unless there is an incompatible break, you can get away with a
# two digit version number, and leave age as 0, always.
#
# Only ever increment the first 2 numbers in this version:
LIB_BARRY_VERSION = 0:13:0

# Disable the default -I. -I$(srcdir) -I$(topdir), etc, but $(top_builddir) is
# needed for config.h
DEFAULT_INCLUDES = -I$(top_builddir)
#INCLUDES = @PACKAGE_CXXFLAGS@ -I@LIBUSB_INC_PATH@
INCLUDES = $(LIBUSB_CFLAGS) $(OPENSSL_CFLAGS)
#AM_CXXFLAGS = -ansi -Wall -fno-strict-aliasing -g -D__DEBUG_MODE__
AM_CFLAGS = -Wall -fno-strict-aliasing -g
AM_CXXFLAGS = -ansi -Wall -fno-strict-aliasing -g
AM_LDFLAGS =
if WITH_GCCVISIBILITY
AM_CFLAGS += -D__BARRY_HAVE_GCCVISIBILITY__ -fvisibility=hidden
AM_CXXFLAGS += -D__BARRY_HAVE_GCCVISIBILITY__ -fvisibility=hidden -fvisibility-inlines-hidden
AM_LDFLAGS += -fvisibility=hidden -fvisibility-inlines-hidden
endif
include_barrydir = ${includedir}/barry

##
## Boost library usage - required for serialization support, but optional
##
#BOOSTFLAG = -D__BARRY_BOOST_MODE__ -I../../../../boost/rootdir/include/boost-1_33_1
#LDBOOST = ../../../../boost/rootdir/lib/libboost_serialization-gcc-mt-1_33_1.a
##BOOSTFLAG =
##LDBOOST =

# Heavy duty C++ warnings
#WARNFLAGS = -ansi -pedantic -Wall -W -Wold-style-cast -Wfloat-equal -Wwrite-strings -Wno-long-long
#WARNFLAGS = -ansi -pedantic -Wall -W -Weffc++ -Woverloaded-virtual -Wold-style-cast -Wfloat-equal -Wwrite-strings -Wno-long-long -Werror

#LDFLAGS = ../../external/rootdir/libusb/lib/libusb.a $(LDBOOST) -lpthread $(LDDEBUG)

lib_LTLIBRARIES = libbarry.la

include_barry_HEADERS = barry.h \
	dll.h \
	builder.h \
	common.h \
	controller.h \
	m_desktop.h \
	m_desktoptmpl.h \
	m_ipmodem.h \
	m_serial.h \
	data.h \
	error.h \
	ldif.h \
	log.h \
	parser.h \
	probe.h \
	protocol.h \
	record.h \
	r_calendar.h \
	r_contact.h \
	r_folder.h \
	r_memo.h \
	r_message.h \
	r_pin_message.h \
	r_saved_message.h \
	r_servicebook.h \
	r_task.h \
	r_timezone.h \
	dataqueue.h \
	router.h \
	socket.h \
	time.h \
	usbwrap.h \
	version.h \
	s11n-boost.h


libbarry_la_SOURCES = time.cc \
	base64.cc base64.h \
	parser.cc \
	data.cc \
	usbwrap.cc \
	probe.cc \
	common.cc \
	error.cc \
	ldif.cc \
	log.cc \
	socket.cc \
	router.cc \
	dataqueue.cc \
	protocol.cc \
	record.cc \
	r_calendar.cc \
	r_contact.cc \
	r_folder.cc \
	r_memo.cc \
	r_message.cc \
	r_pin_message.cc \
	r_saved_message.cc \
	r_servicebook.cc \
	r_task.cc \
	r_timezone.cc \
	packet.cc packet.h \
	controller.cc \
	m_desktop.cc \
	m_ipmodem.cc \
	m_serial.cc \
	version.cc \
	protostructs.h \
	debug.h \
	endian.h
libbarry_la_LIBADD = $(LTLIBOBJS) $(LIBUSB_LIBS) $(OPENSSL_LIBS)
libbarry_la_LDFLAGS = -version-info ${LIB_BARRY_VERSION}

noinst_HEADERS = cbarry.h \
	record-internal.h \
	strnlen.h

EXTRA_DIST = convo.awk \
	Doxyfile \
	legal.txt \
	Makefile.orig

##if DO_TEST
##	bin_PROGRAMS += test-base64 test-data test-time
##
##	test_base64_SOURCES = base64.h base64.cc
##	test_data_SOURCES = data.h data.cc
##	test_time_SOURCES = time.h time.cc
##endif

