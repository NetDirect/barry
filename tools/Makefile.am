EXTRA_DIST = Makefile.plain

#DEFAULT_INCLUDES =
INCLUDES = $(LIBUSB_CFLAGS)
#AM_CXXFLAGS = -ansi -Wall -fno-strict-aliasing -g
AM_CXXFLAGS = -ansi -Wall -g

# To use gettext
datadir = @datadir@
localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

noinst_HEADERS = \
	i18n.h

bin_PROGRAMS = \
	btool \
	bidentify \
	bjavaloader \
	bvncrelay \
	bjvmdebug \
	bdptest \
	bjdwp \
	upldif \
	btranslate \
	bktrans \
	brecsum \
	brimtrans
if WITH_BOOST
bin_PROGRAMS += bs11nread
endif
if WITH_FUSE
bin_PROGRAMS += bfuse
endif

sbin_PROGRAMS = bcharge breset pppob

btool_SOURCES = btool.cc
btool_LDADD = ../src/libbarry.la $(LIBUSB_LIBS) -lpthread @BOOST_LDADD@ $(LTLIBINTL) $(LTLIBICONV)
if WITH_SYNC
btool_LDADD += ../src/libbarrysync.la $(GLIB2_LIBS)
endif
btool_CXXFLAGS = $(AM_CXXFLAGS)
if WITH_BOOST
btool_CXXFLAGS += -D__BARRY_BOOST_MODE__ -D_REENTRANT @BOOST_INC_PATH@
endif
if WITH_SYNC
btool_CXXFLAGS += -D__BARRY_SYNC_MODE__ $(GLIB2_CFLAGS) 
endif
btool_LDFLAGS = @BOOST_LIB_PATH@

if WITH_BOOST
bs11nread_SOURCES = bs11nread.cc
bs11nread_LDADD = ../src/libbarry.la $(LIBUSB_LIBS) -lpthread @BOOST_LDADD@ $(LTLIBINTL)
bs11nread_CXXFLAGS = -D_REENTRANT @BOOST_INC_PATH@ $(AM_CXXFLAGS)
bs11nread_LDFLAGS = @BOOST_LIB_PATH@
endif

bidentify_SOURCES = bidentify.cc
bidentify_LDADD = ../src/libbarry.la $(LIBUSB_LIBS) -lpthread $(LTLIBINTL)

bjavaloader_SOURCES = bjavaloader.cc
bjavaloader_LDADD = ../src/libbarry.la $(LIBUSB_LIBS) -lpthread $(LTLIBINTL)

bvncrelay_SOURCES = bvncrelay.cc
bvncrelay_LDADD = ../src/libbarry.la $(LIBUSB_LIBS) -lpthread

bjvmdebug_SOURCES = bjvmdebug.cc
bjvmdebug_LDADD = ../src/libbarry.la $(LIBUSB_LIBS) -lpthread $(LTLIBINTL)

bdptest_SOURCES = bdptest.cc
bdptest_LDADD = ../src/libbarry.la ../src/libbarrydp.la $(LTLIBINTL)

bjdwp_SOURCES = bjdwp.cc
bjdwp_LDADD = ../src/libbarry.la ../src/libbarryjdwp.la $(LTLIBINTL)

upldif_SOURCES = upldif.cc
upldif_LDADD = ../src/libbarry.la $(LIBUSB_LIBS) -lpthread $(LTLIBINTL)

btranslate_SOURCES = btranslate.cc

bktrans_SOURCES = bktrans.cc
bktrans_LDADD = ../src/libbarry.la -lpthread $(LTLIBINTL)

bcharge_SOURCES = bcharge.cc
bcharge_LDADD = $(LIBUSB_LIBS) $(LTLIBINTL)

breset_SOURCES = breset.cc
breset_LDADD = $(LIBUSB_LIBS) $(LTLIBINTL)

pppob_SOURCES = pppob.cc
pppob_LDADD = ../src/libbarry.la $(LIBUSB_LIBS) -lpthread $(LTLIBINTL)

brecsum_SOURCES = brecsum.cc
brecsum_LDADD = ../src/libbarry.la $(LIBUSB_LIBS) -lpthread $(LTLIBINTL)

if WITH_FUSE
bfuse_SOURCES = bfuse.cc
bfuse_CXXFLAGS = $(FUSE_CFLAGS)
bfuse_LDADD = ../src/libbarry.la $(FUSE_LIBS) -lpthread $(LTLIBINTL)
endif

brimtrans_SOURCES = brimtrans.cc

