bin_PROGRAMS = btool connect translate

INCLUDES	=	-I../libusb/src
CXXFLAGS	+=	-ansi -Wall -fno-strict-aliasing 

btool_SOURCES = btool.cc barry.h \
				time.cc barry_time.h \
				base64.cc base64.h \
				parser.cc parser.h \
				data.cc data.h \
				usbwrap.cc usbwrap.h \
				probe.cc probe.h \
				common.cc common.h \
				error.cc error.h \
				socket.cc socket.h \
				protocol.cc protocol.h \
				record.cc record.h \
				controller.cc controller.h \
				debug.h protostructs.h \
				s11n-boost.h

btool_LDADD		=	 ../libusb/src/.libs/libusb.a

connect_SOURCES = data.h data.cc \
				  usbwrap.h usbwrap.cc \
				  connect.h connect.cc \
				  common.h common.cc

connect_LDADD	=	 ../libusb/src/.libs/libusb.a

translate_SOURCES	=	translate.cc

##if DO_TEST
##	bin_PROGRAMS += test-base64 test-data test-time
##
##	test_base64_SOURCES = base64.h base64.cc
##	test_data_SOURCES = data.h data.cc
##	test_time_SOURCES = time.h time.cc
##endif
