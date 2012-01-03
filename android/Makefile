#
# \file Makefile
#       Make rules for building Barry and dependent libraries for Android
#
#    Copyright (C) 2011-2012 RealVNC Ltd.
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
#    See the GNU General Public License in the COPYING file at the
#    root directory of this project for more details.

LIBUSB_NAME=libusb-1.0.8
LIBICONV_NAME=libiconv-1.13.1
NDK_OUTPUT_FILES=libusb1.0.so lsusb libiconv.so \
	btool brawchannel bidentify bjavaloader bjvmdebug \
	upldif bktrans breset bcharge pppob brecsum
include jni/Application.mk
NDK_OUTPUT_ARCHES=$(APP_ABI)
NDK_BINARIES=$(addprefix libs/, $(foreach ARCH,$(NDK_OUTPUT_ARCHES),$(addprefix $(ARCH)/, $(NDK_OUTPUT_FILES))))
ANDROID_TOOLCHAIN_DIR=android-ndk-tools
ANDROID_NDK_PATH=$(dir $(shell which ndk-build))
WGET=wget

EXTRA_DIST = \
  jni/Android.mk \
  jni/Application.mk \
  jni/BarryTool.mk \
  jni/barry/config.h \
  jni/libusb/config.h \
  README

all: android-binaries

.PHONY: android-binaries

$(LIBUSB_NAME).tar.bz2:
	$(WGET) http://sourceforge.net/projects/libusb/files/libusb-1.0/$(LIBUSB_NAME)/$(LIBUSB_NAME).tar.bz2/download -O $(LIBUSB_NAME).tar.bz2

$(LIBICONV_NAME).tar.gz:
	$(WGET) http://ftp.gnu.org/pub/gnu/libiconv/$(LIBICONV_NAME).tar.gz -O $(LIBICONV_NAME).tar.gz

$(LIBUSB_NAME): $(LIBUSB_NAME).tar.bz2
	tar jxf $(LIBUSB_NAME).tar.bz2
	touch $(LIBUSB_NAME)

$(LIBICONV_NAME): $(LIBICONV_NAME).tar.gz
	tar zxf $(LIBICONV_NAME).tar.gz
	touch $(LIBICONV_NAME)

$(ANDROID_TOOLCHAIN_DIR):
	$(ANDROID_NDK_PATH)/build/tools/make-standalone-toolchain.sh --install-dir="$(ANDROID_TOOLCHAIN_DIR)"

jni/barry_root:
	ln -s ../../ jni/barry_root

jni/barry/barry:
	ln -s ../../../src jni/barry/barry


android-binaries: jni/barry_root jni/barry/barry $(LIBUSB_NAME) $(LIBICONV_NAME) $(ANDROID_TOOLCHAIN_DIR)
	+ANDROID_TOOLCHAIN_DIR="$(realpath $(ANDROID_TOOLCHAIN_DIR))" ndk-build

distclean: clean
	-$(RM) $(LIBUSB_NAME).tar.bz2
	-$(RM) $(LIBICONV_NAME).tar.gz
	-$(RM) -rf libs

clean:
	-$(RM) -rf $(LIBUSB_NAME)
	-$(RM) -rf $(LIBICONV_NAME)
	-$(RM) jni/barry_root
	-$(RM) jni/barry/barry
	-$(RM) -rf obj
	-$(RM) -rf $(NDK_BINARIES)
	-$(RM) -rf $(ANDROID_TOOLCHAIN_DIR)

