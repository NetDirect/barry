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
SYNCBERRYOBJS = \
	data.o \
	usbwrap.o \
	connect.o \


#
# object files needed to build syncberry
#
OBJS = $(SYNCBERRYOBJS)


###############################################################################
all:	visual dep.mak tests connect translate

visual:
	echo ; echo

connect:	$(OBJS)
	$(CXX) $(DEBUG) -o connect $(OBJS) $(LDFLAGS)

translate:	translate.cc
	$(CXX) $(CXXFLAGS) $(DEBUG) -O2 -pg -o translate translate.cc

#############
# Tests
#############

tests:	data-test

data-test:	data.h data.cc
	$(CXX) $(CXXFLAGS) -D__TEST_MODE__ $(DEBUG) -O2 -g -o data-test data.cc

clean-tests:
	rm -f data-test

#################
# Cleanup / Misc
#################

clean:	clean-tests
	rm -f $(OBJS) *.rpo */*.rpo dep.mak core core.* gmon.out leak.out connect
	rm -f translate

dep:
	$(CXX) $(CXXFLAGS) -M *.cc > dep.mak

dep.mak:
	@echo "Please run 'make dep'"
	@echo
	@exit 1

# dependencies.... run make dep to create them
ifeq (dep.mak,$(wildcard dep.mak))
include dep.mak
endif

