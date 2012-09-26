# Android.mk
# This file is the main body of the instructions for how to build one or more native
# libraries for Android using the NDK build system.

LOCAL_PATH:= $(call my-dir)
BARRY_ROOT:= $(LOCAL_PATH)/../../

###############
# libiconv.so #
###############
include $(CLEAR_VARS)

LIBICONV_NAME=libiconv-1.13.1

LOCAL_SRC_FILES := \
  ../$(LIBICONV_NAME)/lib/iconv.c \
  ../$(LIBICONV_NAME)/lib/relocatable.c \
  ../$(LIBICONV_NAME)/libcharset/lib/localcharset.c

LOCAL_C_INCLUDES += \
  $(LOCAL_PATH)/libiconv \
  $(LIBICONV_NAME)/include \
  $(LIBICONV_NAME)/lib \
  $(LIBICONV_NAME)/libcharset/include \
  $(LIBICONV_NAME)/libcharset \
  $(LIBICONV_NAME)/srclib

LOCAL_CFLAGS += -DBUILDING_LIBICONV -DBUILDING_LIBICONV -DIN_LIBRARY -DLIBDIR=\"\"

LOCAL_EXPORT_C_INCLUDES := \
  $(LOCAL_PATH)/../$(LIBICONV_NAME)/include

$(info Checking if libiconv needs configuring...)
COMMAND := $(shell \
           export PATH=$$ANDROID_TOOLCHAIN_DIR/bin:$$PATH; \
           cd $(LOCAL_PATH)/../$(LIBICONV_NAME); \
	   test -e config.h || CC=arm-linux-androideabi-gcc ./configure --host="arm-linux" > configure_run.log 2>&1 )

LOCAL_MODULE := libiconv

include $(BUILD_SHARED_LIBRARY)

################
# libusb1.0.so #
################
include $(CLEAR_VARS)

LIBUSB_NAME=libusb-1.0.8

LOCAL_SRC_FILES := \
  ../$(LIBUSB_NAME)/libusb/core.c \
  ../$(LIBUSB_NAME)/libusb/descriptor.c \
  ../$(LIBUSB_NAME)/libusb/io.c \
  ../$(LIBUSB_NAME)/libusb/sync.c \
  ../$(LIBUSB_NAME)/libusb/os/linux_usbfs.c

LOCAL_C_INCLUDES += \
  $(LOCAL_PATH)/libusb \
  $(LIBUSB_NAME)/libusb/ \
  $(LIBUSB_NAME)/libusb/os/

LOCAL_EXPORT_C_INCLUDES := \
  $(LOCAL_PATH)/../$(LIBUSB_NAME)/libusb

LOCAL_MODULE := libusb1.0

include $(BUILD_SHARED_LIBRARY)

#########
# lsusb #
#########
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  ../$(LIBUSB_NAME)/examples/lsusb.c

LOCAL_C_INCLUDES += \
  $(LOCAL_PATH)/../$(LIBUSB_NAME)

LOCAL_SHARED_LIBRARIES += libusb1.0

LOCAL_MODULE:= lsusb
include $(BUILD_EXECUTABLE)

###############
# libbarry.so #
###############
include $(CLEAR_VARS)

BARRY_SRC_TO_INCLUDE := \
  $(subst $(LOCAL_PATH)/,, \
    $(wildcard $(LOCAL_PATH)/barry_root/src/*.cc) \
  )

BARRY_SRC_TO_EXCLUDE := \
  $(subst $(LOCAL_PATH)/,, \
    $(wildcard $(LOCAL_PATH)/barry_root/src/a_*.cc) \
    $(wildcard $(LOCAL_PATH)/barry_root/src/dp_*.cc) \
    $(wildcard $(LOCAL_PATH)/barry_root/src/j_*.cc) \
    $(wildcard $(LOCAL_PATH)/barry_root/src/v*.cc) \
  ) \
  barry_root/src/mimeio.cc \
  barry_root/src/xmlparser.cc \
  barry_root/src/tzwrapper.cc \
  barry_root/src/tarfile.cc \
  barry_root/src/tarfile-ops-nt.cc \
  barry_root/src/backup.cc \
  barry_root/src/iconvwin.cc \
  barry_root/src/restore.cc \
  barry_root/src/usbwrap_libusb.cc \
  barry_root/src/configfilewin32.cc

LOCAL_SRC_FILES:= $(filter-out $(BARRY_SRC_TO_EXCLUDE), $(BARRY_SRC_TO_INCLUDE)) barry_root/src/version.cc
LOCAL_C_INCLUDES += \
  $(LOCAL_PATH)/barry \
  $(LOCAL_PATH)/../$(LIBUSB_NAME)/libusb

LOCAL_CFLAGS += -D__BARRY_LIBRARY_BUILD__ -DLOCALEDIR=\"\"

LOCAL_EXPORT_C_INCLUDES := \
  $(LOCAL_PATH)/barry

LOCAL_CPP_EXTENSION := .cc
LOCAL_SHARED_LIBRARIES += libusb1.0 libiconv
LOCAL_LDLIBS := -lz

LOCAL_MODULE := libbarry

# Have to build libbarry statically as libstdc++ is statically linked in
include $(BUILD_STATIC_LIBRARY)

#########
# tools #
#########
TOOLS=brawchannel btool bidentify bjavaloader bjvmdebug upldif bktrans pppob brecsum

$(foreach TOOL, $(TOOLS), $(eval include $(LOCAL_PATH)/BarryTool.mk))

###########
# bcharge #
###########
include $(CLEAR_VARS)

LOCAL_SRC_FILES := barry_root/tools/bcharge_libusb_1_0.cc

LOCAL_CFLAGS += -DLOCALEDIR=\"\"
LOCAL_CPP_EXTENSION := .cc
LOCAL_C_INCLUDES += \
  $(LOCAL_PATH)/barry

LOCAL_SHARED_LIBRARIES += libusb1.0

LOCAL_MODULE := bcharge

include $(BUILD_EXECUTABLE)

###########
# breset #
###########
include $(CLEAR_VARS)

LOCAL_SRC_FILES := barry_root/tools/breset_libusb_1_0.cc

LOCAL_CFLAGS += -DLOCALEDIR=\"\"
LOCAL_CPP_EXTENSION := .cc
LOCAL_C_INCLUDES += \
  $(LOCAL_PATH)/barry 

LOCAL_SHARED_LIBRARIES += libusb1.0

LOCAL_MODULE := breset

include $(BUILD_EXECUTABLE)