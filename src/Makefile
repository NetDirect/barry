#
# Makefile for the Syncberry library and test programs
#

#
# debug flags  (note: __DEBUG_MODE__ also turns assert's NDEBUG off and on
#               appropriately)
#
#DEBUG = -g -pg -D__DEBUG_MODE__
#DEBUG = -g -D__DEBUG_MODE__
DEBUG = -g
#DEBUG = -D__DEBUG_MODE__
#DEBUG =
#LDDEBUG = -u malloc -lefence
#LDDEBUG = -u malloc -u free -ldmalloc
LDDEBUG =

#
# include directories to various libraries we use
#
LIBUSBINC = ../../external/rootdir/libusb/include
INCLUDE = -I$(LIBUSBINC)

#
# optimization flags
# http://gcc.gnu.org/ml/gcc/2000-04/msg00377.html
# http://gcc.gnu.org/ml/gcc-help/2004-12/msg00142.html
# http://gcc.gnu.org/ml/gcc-help/2004-12/msg00147.html
# (current winner is at the top)
#
#OPTFLAGS = -frepo -fdata-sections -ffunction-sections
#OPTFLAGS = -frepo -fdata-sections
OPTFLAGS =
#OPTFLAGS = -frepo
#OPTFLAGS = -fno-enforce-eh-specs -fno-default-inline -fno-implement-inlines
#OPTFLAGS = -fno-inline
#OPTFLAGS = -fno-implicit-templates
#OPTFLAGS = -Os
#OPTFLAGS = -Os -fomit-frame-pointer
#OPTFLAGS = -Os -fomit-frame-pointer -mcpu=i386

#
# warning and error checking flags
#
#WARNFLAGS =
WARNFLAGS = -ansi -Wall
#WARNFLAGS = -ansi -pedantic -Wall -W -Wold-style-cast -Wfloat-equal -Wwrite-strings -Wno-long-long
#WARNFLAGS = -ansi -pedantic -Wall -W -Weffc++ -Woverloaded-virtual -Wold-style-cast -Wfloat-equal -Wwrite-strings -Wno-long-long -Werror

#
# compiler flags
#
CXX = g++
CXXFLAGS = $(WARNFLAGS) $(OPTFLAGS) $(INCLUDE) $(SPECIAL) $(DEBUG)
#LDFLAGS = -s -lmysqlpp -lfaxutil -lldap $(LDDEBUG)
LDFLAGS = ../../external/rootdir/libusb/lib/libusb.a -lpthread $(LDDEBUG)
#LDFLAGS = -Wl,--gc-sections -s -lmysqlpp -lfaxutil -lldap $(LDDEBUG)

#
# Syncberry files
#
CONNECTOBJS = \
	data.o \
	usbwrap.o \
	connect.o \


BBTOOLOBJS = \
	data.o \
	usbwrap.o \
	bbtool.o \
	probe.o \
	sbcommon.o \
	error.o \
	socket.o \
	protocol.o \
	record.o \
	blackberry.o \


#
# object files needed to build syncberry
#
OBJS = $(CONNECTOBJS) $(BBTOOLOBJS)


###############################################################################
all:	visual dep.mak tests connect bbtool translate

visual:
	echo ; echo

connect:	$(CONNECTOBJS)
	$(CXX) $(DEBUG) -o connect $(CONNECTOBJS) $(LDFLAGS)

bbtool:		$(BBTOOLOBJS)
	$(CXX) $(DEBUG) -o bbtool $(BBTOOLOBJS) $(LDFLAGS)

translate:	translate.cc
	$(CXX) $(CXXFLAGS) -o translate translate.cc

#############
# Tests
#############

tests:	test-data test-contact

test-data:	data.h data.cc
	$(CXX) $(CXXFLAGS) -D__TEST_MODE__ -o test-data data.cc

test-contact.o:	record.cc
	$(CXX) $(CXXFLAGS) -D__TEST_MODE__ -o test-contact.o -c record.cc

test-contact:	test-contact.o data.o
	$(CXX) $(CXXFLAGS) -o test-contact test-contact.o data.o

clean-tests:
	rm -f test-data test-contact test-contact.o

#################
# Cleanup / Misc
#################

clean:	clean-tests
	rm -f $(OBJS) *.rpo */*.rpo dep.mak core core.* gmon.out leak.out
	rm -f connect bbtool
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

