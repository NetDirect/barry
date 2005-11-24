#
# Makefile for the Barry library and test programs
#

include ../Makefile.conf

#
# Barry files
#
CONNECTOBJS = \
	data.o \
	usbwrap.o \
	connect.o \


BBTOOLOBJS = \
	time.o \
	base64.o \
	parser.o \
	data.o \
	usbwrap.o \
	btool.o \
	probe.o \
	common.o \
	error.o \
	socket.o \
	protocol.o \
	record.o \
	controller.o \


#
# object files needed to build Barry
#
OBJS = $(CONNECTOBJS) $(BBTOOLOBJS)


###############################################################################
all:	visual dep.mak tests connect btool translate

visual:
	echo ; echo

connect:	$(CONNECTOBJS)
	$(CXX) $(DEBUG) -o connect $(CONNECTOBJS) $(LDFLAGS)

btool:		$(BBTOOLOBJS)
	$(CXX) $(DEBUG) -o btool $(BBTOOLOBJS) $(LDFLAGS)

translate:	translate.cc
	$(CXX) $(CXXFLAGS) -o translate translate.cc

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
	rm -f test-data test-contact test-contact.o

#################
# Cleanup / Misc
#################

clean:	clean-tests
	rm -f $(OBJS) *.rpo */*.rpo dep.mak core core.* gmon.out leak.out
	rm -f connect btool
	rm -f translate
	rm -rf ../doc/doxygen/html

dep:
	$(CXX) $(CXXFLAGS) -M *.cc > dep.mak

dep.mak:
	make dep
	make

# dependencies.... run make dep to create them
ifeq (dep.mak,$(wildcard dep.mak))
include dep.mak
endif

