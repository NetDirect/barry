
#DEFAULT_INCLUDES =
INCLUDES = $(LIBUSB_CFLAGS)
#AM_CXXFLAGS = -ansi -Wall -fno-strict-aliasing -g
AM_CXXFLAGS = -ansi -Wall -g

bin_PROGRAMS = btool upldif translate
sbin_PROGRAMS = bcharge

btool_SOURCES = btool.cc
btool_LDADD = ../src/libbarry.la
if WITH_BOOST
btool_CXXFLAGS = -D__BARRY_BOOST_MODE__ -D_REENTRANT -I@BOOST_INC_PATH@ $(AM_CXXFLAGS)
btool_LDFLAGS = -L@BOOST_LIB_PATH@
#btool_LIBADD = libboost_serialization-gcc-mt.a
#btool_LDADD += @BOOST_LIB_PATH@/libboost_serialization.a
btool_LDADD += -lboost_serialization
#LDBOOST = ../../../../boost/rootdir/lib/libboost_serialization-gcc-mt-1_33_1.a
endif

upldif_SOURCES = upldif.cc
upldif_LDADD = ../src/libbarry.la

translate_SOURCES = translate.cc

bcharge_SOURCES = bcharge.cc
bcharge_LDADD = $(LIBUSB_LIBS)

