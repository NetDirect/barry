#
# Makefile for the Barry library and test programs
#

include ../Makefile.conf

#
# Barry files
#
LIBOBJS = \
	time.o \
	base64.o \
	parser.o \
	data.o \
	usbwrap.o \
	probe.o \
	common.o \
	error.o \
	socket.o \
	protocol.o \
	record.o \
	packet.o \
	controller.o

CONNECTOBJS = \
	data.o \
	usbwrap.o \
	connect.o \
	common.o \


BBTOOLOBJS = \
	btool.o \
	libbarry.a

UPLDIFOBJS = \
	time.o \
	base64.o \
	parser.o \
	data.o \
	usbwrap.o \
	probe.o \
	common.o \
	error.o \
	socket.o \
	protocol.o \
	record.o \
	packet.o \
	controller.o \
	upldif.o


#
# object files needed to build Barry
#
OBJS = $(LIBOBJS) $(CONNECTOBJS) $(BBTOOLOBJS) $(UPLDIFOBJS)


###############################################################################
all:	visual dep.mak tests connect btool upldif translate libbarry.a

visual:
	echo ; echo

connect:	$(CONNECTOBJS)
	$(CXX) $(DEBUG) -o connect $(CONNECTOBJS) $(LDFLAGS)

btool:		$(BBTOOLOBJS)
	$(CXX) $(DEBUG) -o btool $(BBTOOLOBJS) $(LDFLAGS)

upldif:		$(UPLDIFOBJS)
	$(CXX) $(DEBUG) -o upldif $(UPLDIFOBJS) $(LDFLAGS)

translate:	translate.cc
	$(CXX) $(CXXFLAGS) -o translate translate.cc

libbarry.a:	$(LIBOBJS)
	ar rcs libbarry.a $(LIBOBJS)

#############
# Install
#############

install:
	mkdir -p $(INSTALLDIR)
	mkdir -p $(INSTALLDIR)/include/barry
	mkdir -p $(INSTALLDIR)/lib
	install libbarry.a $(INSTALLDIR)/lib
	install barry.h base64.h builder.h common.h controller.h controllertmpl.h data.h debug.h error.h packet.h parser.h probe.h protocol.h protostructs.h record.h s11n-boost.h socket.h time.h usbwrap.h $(INSTALLDIR)/include/barry


#############
# Tests
#############

tests:	test-data test-contact test-base64 test-time

test-time:	time.h time.cc
	$(CXX) $(CXXFLAGS) -D__TEST_MODE__ -o test-time time.cc

test-base64:	base64.h base64.cc
	$(CXX) $(CXXFLAGS) -D__TEST_MODE__ -o test-base64 base64.cc

test-data:	data.h data.cc
	$(CXX) $(CXXFLAGS) -D__TEST_MODE__ -o test-data data.cc

test-contact.o:	record.cc
	$(CXX) $(CXXFLAGS) -D__TEST_MODE__ -o test-contact.o -c record.cc

test-contact:	test-contact.o data.o base64.o
	$(CXX) $(CXXFLAGS) -o test-contact test-contact.o data.o base64.o

clean-tests:
	rm -f test-data test-contact test-contact.o test-base64 test-time

#################
# Cleanup / Misc
#################

clean:	clean-tests
	rm -f $(OBJS) *.rpo */*.rpo dep.mak core core.* gmon.out leak.out
	rm -f connect btool upldif
	rm -f translate
	rm -f libbarry.a
	rm -rf ../doc/doxygen/html
	rm -rf install

dep:
	$(CXX) $(CXXFLAGS) -M *.cc > dep.mak

dep.mak:
	make dep
	make

# dependencies.... run make dep to create them
ifeq (dep.mak,$(wildcard dep.mak))
include dep.mak
endif

