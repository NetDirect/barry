EXTRA_DIST = buildtest.sh nightly.sh

#DEFAULT_INCLUDES =
INCLUDES =
#AM_CXXFLAGS = -ansi -Wall -fno-strict-aliasing -g
AM_CXXFLAGS = -Wall -g

# To use gettext
datadir = @datadir@
localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

noinst_HEADERS = libtest.h

noinst_PROGRAMS = libtest fhbuild ptyio

libtest_SOURCES = \
	date.cc \
	data.cc \
	libtest.cc
libtest_LDADD = \
	../src/libbarry.la \
	@BOOST_LDADD@ $(LTLIBINTL) $(LTLIBICONV)
libtest_LDFLAGS = @BOOST_LIB_PATH@
libtest_CXXFLAGS = $(AM_CXXFLAGS)

if WITH_BOOST
libtest_CXXFLAGS += -D__BARRY_BOOST_MODE__ -D_REENTRANT @BOOST_INC_PATH@
endif
if WITH_SYNC
libtest_LDADD += ../src/libbarrysync.la $(GLIB2_LIBS)
libtest_CXXFLAGS += -D__BARRY_SYNC_MODE__ $(GLIB2_CFLAGS) 
endif
if WITH_BACKUP
libtest_LDADD += ../src/libbarrybackup.la
libtest_CXXFLAGS += -D__BARRY_BACKUP_MODE__
endif

fhbuild_SOURCES = fhbuild.cc
fhbuild_LDADD = ../src/libbarry.la $(LTLIBINTL) $(LTLIBICONV)
fhbuild_CXXFLAGS = $(AM_CXXFLAGS)

ptyio_SOURCES = ptyio.cc
ptyio_CXXFLAGS = $(AM_CXXFLAGS)

