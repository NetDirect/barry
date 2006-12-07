
#DEFAULT_INCLUDES =
INCLUDES = -I@LIBUSB_INC_PATH@
#AM_CXXFLAGS = -ansi -Wall -fno-strict-aliasing -g
AM_CXXFLAGS = -ansi -Wall -g

bin_PROGRAMS = btool upldif translate

btool_SOURCES = btool.cc
btool_LDADD = ../src/libbarry.la

upldif_SOURCES = upldif.cc
upldif_LDADD = ../src/libbarry.la

translate_SOURCES = translate.cc

