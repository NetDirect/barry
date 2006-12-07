
# Disable the default -I. -I$(srcdir) -I$(topdir), etc, as our includes
# are proper anyway. :-)
DEFAULT_INCLUDES =
#INCLUDES = @PACKAGE_CXXFLAGS@ -I@LIBUSB_INC_PATH@
INCLUDES = -I@LIBUSB_INC_PATH@
AM_CXXFLAGS = -ansi -Wall -fno-strict-aliasing -g
include_barrydir = ${includedir}/barry

##
## Boost library usage - required for serialization support, but optional
##
#BOOSTFLAG = -D__BOOST_MODE__ -I../../../../boost/rootdir/include/boost-1_33_1
#LDBOOST = ../../../../boost/rootdir/lib/libboost_serialization-gcc-mt-1_33_1.a
##BOOSTFLAG =
##LDBOOST =

# Heavy duty C++ warnings
#WARNFLAGS = -ansi -pedantic -Wall -W -Wold-style-cast -Wfloat-equal -Wwrite-strings -Wno-long-long
#WARNFLAGS = -ansi -pedantic -Wall -W -Weffc++ -Woverloaded-virtual -Wold-style-cast -Wfloat-equal -Wwrite-strings -Wno-long-long -Werror

#LDFLAGS = ../../external/rootdir/libusb/lib/libusb.a $(LDBOOST) -lpthread $(LDDEBUG)

lib_LTLIBRARIES = libbarry.la

include_barry_HEADERS = barry.h \
	builder.h \
	common.h \
	controller.h \
	controllertmpl.h \
	data.h \
	error.h \
	parser.h \
	probe.h \
	protocol.h \
	record.h \
	socket.h \
	usbwrap.h \
	time.h


libbarry_la_SOURCES = time.cc \
	base64.cc base64.h \
	parser.cc \
	data.cc \
	usbwrap.cc \
	probe.cc \
	common.cc \
	error.cc \
	socket.cc \
	protocol.cc \
	record.cc \
	packet.cc packet.h \
	controller.cc \
	protostructs.h \
	debug.h \
	endian.h
libbarry_la_LIBADD = @LIBUSB_LIB_PATH@/libusb.la

##if DO_TEST
##	bin_PROGRAMS += test-base64 test-data test-time
##
##	test_base64_SOURCES = base64.h base64.cc
##	test_data_SOURCES = data.h data.cc
##	test_time_SOURCES = time.h time.cc
##endif

