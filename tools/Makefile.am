EXTRA_DIST = Makefile.plain

#DEFAULT_INCLUDES =
INCLUDES = $(LIBUSB_CFLAGS)
#AM_CXXFLAGS = -ansi -Wall -fno-strict-aliasing -g
AM_CXXFLAGS = -ansi -Wall -g

bin_PROGRAMS = btool bidentify upldif btranslate bktrans
if WITH_BOOST
bin_PROGRAMS += bs11nread
endif
sbin_PROGRAMS = bcharge breset pppob

btool_SOURCES = btool.cc
btool_LDADD = ../src/libbarry.la $(LIBUSB_LIBS) -lpthread
if WITH_BOOST
if WITH_BOOST_PATHS
btool_CXXFLAGS = -D__BARRY_BOOST_MODE__ -D_REENTRANT -I@BOOST_INC_PATH@ $(AM_CXXFLAGS)
btool_LDFLAGS = -L@BOOST_LIB_PATH@
else
btool_CXXFLAGS = -D__BARRY_BOOST_MODE__ -D_REENTRANT $(AM_CXXFLAGS)
endif
btool_LDADD += -lboost_serialization
endif

if WITH_BOOST
bs11nread_SOURCES = bs11nread.cc
bs11nread_LDADD = ../src/libbarry.la $(LIBUSB_LIBS) -lpthread
if WITH_BOOST_PATHS
bs11nread_CXXFLAGS = -D_REENTRANT -I@BOOST_INC_PATH@ $(AM_CXXFLAGS)
bs11nread_LDFLAGS = -L@BOOST_LIB_PATH@
else
bs11nread_CXXFLAGS = -D_REENTRANT $(AM_CXXFLAGS)
endif
bs11nread_LDADD += -lboost_serialization
endif

bidentify_SOURCES = bidentify.cc
bidentify_LDADD = ../src/libbarry.la $(LIBUSB_LIBS) -lpthread

upldif_SOURCES = upldif.cc
upldif_LDADD = ../src/libbarry.la $(LIBUSB_LIBS) -lpthread

btranslate_SOURCES = btranslate.cc

bktrans_SOURCES = bktrans.cc
bktrans_LDADD = ../src/libbarry.la -lpthread

bcharge_SOURCES = bcharge.cc
bcharge_LDADD = $(LIBUSB_LIBS)

breset_SOURCES = breset.cc
breset_LDADD = $(LIBUSB_LIBS)

pppob_SOURCES = pppob.cc
pppob_LDADD = ../src/libbarry.la $(LIBUSB_LIBS) -lpthread

