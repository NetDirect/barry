EXTRA_DIST = brawchannel_win32.cc

#DEFAULT_INCLUDES =
INCLUDES = $(USB_LIBRARY_CFLAGS)
#AM_CXXFLAGS = -ansi -Wall -fno-strict-aliasing -g
AM_CXXFLAGS = -Wall -g

# To use gettext
datadir = @datadir@
localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

noinst_HEADERS = \
	i18n.h \
	gettext.h \
	brecsum.h \
	util.h \
	boostwrap.h \
	barrygetopt.h \
	platform.h \
	brawchannel.h

noinst_PROGRAMS = \
	btranslate \
	bktrans \
	brimtrans

noinst_LIBRARIES = \
	libutil.a \
	libboostwrap.a

bin_PROGRAMS = \
	btool \
	bidentify \
	bjavaloader \
	brawchannel \
	bjvmdebug \
	bdptest \
	bjdwp \
	upldif \
	brecsum
if WITH_BOOST
bin_PROGRAMS += bs11nread
endif
if WITH_FUSE
bin_PROGRAMS += bfuse
endif
if WITH_SDL
bin_PROGRAMS += bwatch
endif
if WITH_BACKUP
bin_PROGRAMS += btardump btarcmp
endif
if WITH_ALX
bin_PROGRAMS += balxparse
endif
if WITH_BACKUP
if WITH_SYNC
bin_PROGRAMS += bio
endif
endif

sbin_PROGRAMS = \
	bcharge \
	breset \
	pppob

libutil_a_SOURCES = util.cc
libutil_a_CXXFLAGS = $(AM_CXXFLAGS)
if WITH_SYNC
libutil_a_CXXFLAGS += -D__BARRY_SYNC_MODE__ $(GLIB2_CFLAGS)
endif

libboostwrap_a_SOURCES = boostwrap.cc
libboostwrap_a_CXXFLAGS = $(AM_CXXFLAGS)
if WITH_BOOST
libboostwrap_a_CXXFLAGS += -D__BARRY_BOOST_MODE__ -D_REENTRANT @BOOST_INC_PATH@
endif

btool_SOURCES = btool.cc
btool_LDADD = ../src/libbarry.la libutil.a libboostwrap.a $(USB_LIBRARY_LIBS) @BOOST_LDADD@ $(LTLIBINTL) $(LTLIBICONV)
if WITH_SYNC
btool_LDADD += ../src/libbarrysync.la $(GLIB2_LIBS)
endif
if WITH_BACKUP
btool_LDADD += ../src/libbarrybackup.la
endif
btool_CXXFLAGS = $(AM_CXXFLAGS)
if WITH_BOOST
btool_CXXFLAGS += -D__BARRY_BOOST_MODE__ -D_REENTRANT @BOOST_INC_PATH@
endif
if WITH_SYNC
btool_CXXFLAGS += -D__BARRY_SYNC_MODE__ $(GLIB2_CFLAGS)
endif
if WITH_BACKUP
btool_CXXFLAGS += -D__BARRY_BACKUP_MODE__
endif
btool_LDFLAGS = @BOOST_LIB_PATH@

if WITH_BOOST
bs11nread_SOURCES = bs11nread.cc
bs11nread_LDADD = ../src/libbarry.la libutil.a libboostwrap.a $(USB_LIBRARY_LIBS) @BOOST_LDADD@ $(LTLIBINTL)
bs11nread_CXXFLAGS = -D_REENTRANT @BOOST_INC_PATH@ $(AM_CXXFLAGS)
bs11nread_LDFLAGS = @BOOST_LIB_PATH@
endif

bidentify_SOURCES = bidentify.cc
bidentify_LDADD = ../src/libbarry.la $(USB_LIBRARY_LIBS) $(LTLIBINTL)

bjavaloader_SOURCES = bjavaloader.cc
bjavaloader_LDADD = ../src/libbarry.la $(USB_LIBRARY_LIBS) $(LTLIBINTL)

brawchannel_SOURCES = brawchannel.cc brawchannel_unix.cc
brawchannel_LDADD = ../src/libbarry.la $(USB_LIBRARY_LIBS) $(LTLIBINTL)

bjvmdebug_SOURCES = bjvmdebug.cc
bjvmdebug_LDADD = ../src/libbarry.la $(USB_LIBRARY_LIBS) $(LTLIBINTL)

bdptest_SOURCES = bdptest.cc
bdptest_LDADD = ../src/libbarry.la ../src/libbarrydp.la $(LTLIBINTL)

bjdwp_SOURCES = bjdwp.cc
bjdwp_LDADD = ../src/libbarry.la \
	../src/libbarrydp.la \
	../src/libbarryjdwp.la \
	$(LTLIBINTL)

upldif_SOURCES = upldif.cc
upldif_LDADD = ../src/libbarry.la $(USB_LIBRARY_LIBS) $(LTLIBINTL)

btranslate_SOURCES = btranslate.cc
btranslate_LDADD = $(LTLIBINTL)

bktrans_SOURCES = bktrans.cc
bktrans_LDADD = ../src/libbarry.la $(LTLIBINTL)

if USE_LIBUSB_0_1
bcharge_SOURCES = bcharge.cc
bcharge_LDADD = $(USB_LIBRARY_LIBS) $(LTLIBINTL)

breset_SOURCES = breset.cc
breset_LDADD = $(USB_LIBRARY_LIBS) $(LTLIBINTL)
endif

if USE_LIBUSB_1_0
bcharge_SOURCES = bcharge_libusb_1_0.cc
bcharge_LDADD = $(USB_LIBRARY_LIBS) $(LTLIBINTL)

breset_SOURCES = breset_libusb_1_0.cc
breset_LDADD = $(USB_LIBRARY_LIBS) $(LTLIBINTL)
endif

pppob_SOURCES = pppob.cc
pppob_LDADD = ../src/libbarry.la $(USB_LIBRARY_LIBS) $(LTLIBINTL)

brecsum_SOURCES = brecsum.cc
brecsum_LDADD = ../src/libbarry.la $(USB_LIBRARY_LIBS) $(LTLIBINTL)

brimtrans_SOURCES = brimtrans.cc
brimtrans_LDADD = $(LTLIBINTL)

if WITH_FUSE
bfuse_SOURCES = bfuse.cc
bfuse_CXXFLAGS = $(FUSE_CFLAGS)
bfuse_LDADD = ../src/libbarry.la $(FUSE_LIBS) $(LTLIBINTL)
endif

if WITH_SDL
bwatch_SOURCES = bwatch.cc
bwatch_CXXFLAGS = $(SDL_FLAGS)
bwatch_LDADD = ../src/libbarry.la $(SDL_LIBS) $(LTLIBINTL)
endif

if WITH_BACKUP
btardump_SOURCES = btardump.cc
btardump_CXXFLAGS = $(AM_CXXFLAGS)
#if WITH_BOOST
#btardump_CXXFLAGS += -D__BARRY_BOOST_MODE__ -D_REENTRANT @BOOST_INC_PATH@
#endif
if WITH_SYNC
btardump_CXXFLAGS += -D__BARRY_SYNC_MODE__ $(GLIB2_CFLAGS)
endif
btardump_LDADD = ../src/libbarry.la $(USB_LIBRARY_LIBS) \
	../src/libbarrybackup.la $(LTLIBINTL)
if WITH_SYNC
btardump_LDADD += ../src/libbarrysync.la $(GLIB2_LIBS)
endif
endif

if WITH_BACKUP
btarcmp_SOURCES = btarcmp.cc
btarcmp_CXXFLAGS = $(AM_CXXFLAGS)
btarcmp_LDADD = ../src/libbarry.la ../src/libbarrybackup.la libutil.a \
			$(USB_LIBRARY_LIBS) $(LTLIBINTL)
if WITH_SYNC
btarcmp_CXXFLAGS += -D__BARRY_SYNC_MODE__ $(GLIB2_CFLAGS)
endif
endif

if WITH_ALX
balxparse_SOURCES = balxparse.cc
balxparse_CXXFLAGS = $(AM_CXXFLAGS) $(LIBXMLXX_CFLAGS)
balxparse_LDADD = ../src/libbarry.la ../src/libbarryalx.la $(LTLIBINTL)
endif

if WITH_BACKUP
if WITH_SYNC
bio_SOURCES = bio.cc
bio_CXXFLAGS = -D__BARRY_SYNC_MODE__ $(AM_CXXFLAGS) $(GLIB2_CFLAGS)
if WITH_BOOST
bio_CXXFLAGS += -D__BARRY_BOOST_MODE__ -D_REENTRANT @BOOST_INC_PATH@
endif
bio_LDADD = ../src/libbarry.la \
	../src/libbarrysync.la $(GLIB2_LIBS) \
	../src/libbarrybackup.la \
	libutil.a \
	libboostwrap.a \
	$(USB_LIBRARY_LIBS) @BOOST_LDADD@ \
	$(LTLIBINTL) $(LTLIBICONV)
bio_LDFLAGS = @BOOST_LIB_PATH@
endif
endif

