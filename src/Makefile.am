
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
LIB_BARRY_VERSION = 0:16:0

# Disable the default -I. -I$(srcdir) -I$(topdir), etc, but $(top_builddir) is
# needed for config.h
DEFAULT_INCLUDES = -I$(top_builddir)
#INCLUDES = @PACKAGE_CXXFLAGS@ -I@LIBUSB_INC_PATH@
#INCLUDES = $(LIBUSB_CFLAGS) $(OPENSSL_CFLAGS)
INCLUDES = $(LIBUSB_CFLAGS)
#AM_CXXFLAGS = -ansi -Wall -fno-strict-aliasing -g -D__DEBUG_MODE__
AM_CFLAGS = -Wall -fno-strict-aliasing -g -D__BARRY_LIBRARY_BUILD__
AM_CXXFLAGS = -ansi -Wall -fno-strict-aliasing -g -D__BARRY_LIBRARY_BUILD__
AM_LDFLAGS =
if WITH_GCCVISIBILITY
AM_CFLAGS += -D__BARRY_HAVE_GCCVISIBILITY__ -fvisibility=hidden
#AM_CXXFLAGS += -D__BARRY_HAVE_GCCVISIBILITY__ -fvisibility=hidden -fvisibility-inlines-hidden
AM_CXXFLAGS += -D__BARRY_HAVE_GCCVISIBILITY__ -fvisibility=hidden
#AM_LDFLAGS += -fvisibility=hidden -fvisibility-inlines-hidden
AM_LDFLAGS += -fvisibility=hidden
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

#
# libbarry - main USB protocol interface library
# libbarrydp - '.debug' parser library
# libbarryjdwp - java debug server library
#
lib_LTLIBRARIES = libbarry.la libbarrydp.la libbarryjdwp.la

include_barry_HEADERS = barry.h \
	dll.h \
	builder.h \
	common.h \
	controller.h \
	m_mode_base.h \
	m_desktop.h \
	m_desktoptmpl.h \
	m_ipmodem.h \
	m_serial.h \
	m_javaloader.h \
	m_jvmdebug.h \
	data.h \
	error.h \
	ldif.h \
	log.h \
	parser.h \
	probe.h \
	protocol.h \
	record.h \
	modem.h \
	r_recur_base.h \
	r_calendar.h \
	r_calllog.h \
	r_contact.h \
	r_folder.h \
	r_memo.h \
	r_message_base.h \
	r_message.h \
	r_pin_message.h \
	r_saved_message.h \
	r_servicebook.h \
	r_sms.h \
	r_task.h \
	r_timezone.h \
	dataqueue.h \
	router.h \
	socket.h \
	time.h \
	threadwrap.h \
	usbwrap.h \
	version.h \
	pppfilter.h \
	sha1.h \
	iconv.h \
	cod.h \
	bmp.h \
	s11n-boost.h \
	dp_codinfo.h \
	j_manager.h \
	j_server.h

noinst_HEADERS = cbarry.h \
	base64.h \
	record-internal.h \
	bmp-internal.h \
	cod-internal.h \
	packet.h \
	scoped_lock.h \
	protostructs.h \
	debug.h \
	endian.h \
	strnlen.h \
	dp_parser.h \
	j_message.h \
	j_jdwp.h

EXTRA_DIST = convo.awk \
	legal.txt

libbarry_la_SOURCES = time.cc \
	base64.cc \
	bmp.cc \
	cod.cc \
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
	threadwrap.cc \
	protocol.cc \
	record.cc \
	r_recur_base.cc \
	r_calendar.cc \
	r_calllog.cc \
	r_command.cc \
	r_contact.cc \
	r_dbdb.cc \
	r_folder.cc \
	r_memo.cc \
	r_message_base.cc \
	r_recordstate.cc \
	r_servicebook.cc \
	r_sms.cc \
	r_task.cc \
	r_timezone.cc \
	packet.cc \
	controller.cc \
	m_mode_base.cc \
	m_desktop.cc \
	m_ipmodem.cc \
	m_serial.cc \
	m_javaloader.cc \
	m_jvmdebug.cc \
	version.cc \
	pppfilter.cc \
	sha1.cc \
	iconv.cc
#libbarry_la_LIBADD = $(LTLIBOBJS) $(LIBUSB_LIBS) $(OPENSSL_LIBS)
libbarry_la_LIBADD = $(LTLIBOBJS) $(LIBUSB_LIBS) $(ZLIB_LIBS) @LTLIBICONV@
libbarry_la_LDFLAGS = -version-info ${LIB_BARRY_VERSION}

libbarrydp_la_SOURCES = \
	dp_parser.cc \
	dp_codinfo.cc
libbarrydp_la_LIBADD = libbarry.la
libbarrydp_la_LDFLAGS = -version-info ${LIB_BARRY_VERSION}

libbarryjdwp_la_SOURCES = \
	j_record.cc \
	j_jdwp.cc \
	j_message.cc \
	j_server.cc \
	j_manager.cc
libbarryjdwp_la_LIBADD = libbarry.la libbarrydp.la $(LIBUSB_LIBS)
libbarryjdwp_la_LDFLAGS = -version-info ${LIB_BARRY_VERSION}

##if DO_TEST
##	bin_PROGRAMS += test-base64 test-data test-time
##
##	test_base64_SOURCES = base64.h base64.cc
##	test_data_SOURCES = data.h data.cc
##	test_time_SOURCES = time.h time.cc
##endif

