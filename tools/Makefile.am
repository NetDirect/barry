EXTRA_DIST = Makefile.plain

#DEFAULT_INCLUDES =
INCLUDES = $(LIBUSB_CFLAGS)
#AM_CXXFLAGS = -ansi -Wall -fno-strict-aliasing -g
AM_CXXFLAGS = -ansi -Wall -g

bin_PROGRAMS = btool bidentify upldif btranslate bktrans
sbin_PROGRAMS = bcharge breset pppob

btool_SOURCES = btool.cc
btool_LDADD = ../src/libbarry.la $(LIBUSB_LIBS)
if WITH_BOOST
btool_CXXFLAGS = -D__BARRY_BOOST_MODE__ -D_REENTRANT -I@BOOST_INC_PATH@ $(AM_CXXFLAGS)
btool_LDFLAGS = -L@BOOST_LIB_PATH@
#btool_LIBADD = libboost_serialization-gcc-mt.a
#btool_LDADD += @BOOST_LIB_PATH@/libboost_serialization.a
btool_LDADD += -lboost_serialization
#LDBOOST = ../../../../boost/rootdir/lib/libboost_serialization-gcc-mt-1_33_1.a
endif

bidentify_SOURCES = bidentify.cc
bidentify_LDADD = ../src/libbarry.la $(LIBUSB_LIBS)

upldif_SOURCES = upldif.cc
upldif_LDADD = ../src/libbarry.la $(LIBUSB_LIBS)

btranslate_SOURCES = btranslate.cc

bktrans_SOURCES = bktrans.cc
bktrans_LDADD = ../src/libbarry.la

bcharge_SOURCES = bcharge.cc
bcharge_LDADD = $(LIBUSB_LIBS)

breset_SOURCES = breset.cc
breset_LDADD = $(LIBUSB_LIBS)

pppob_SOURCES = pppob.cc
pppob_LDADD = ../src/libbarry.la $(LIBUSB_LIBS) -lpthread

