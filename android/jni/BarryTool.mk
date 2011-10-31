# BarryTool.mk
# This file is used to build a single Barry tool. It takes the following parameters:
#   TOOL - The name of the tool to make
#   LOCAL_PATH - Path to the JNI directory

include $(CLEAR_VARS)

LOCAL_SRC_FILES := barry_root/tools/$(TOOL).cc

LOCAL_C_INCLUDES += \
  $(LOCAL_PATH)/barry

LOCAL_CPP_EXTENSION := cc

LOCAL_SHARED_LIBRARIES += libbarry

LOCAL_MODULE := $(TOOL)

include $(BUILD_EXECUTABLE)