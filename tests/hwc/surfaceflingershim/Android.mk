LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

## Manage software and hardware variants.
VAL_HWC_HWC_COMMON_INC_PATH:=$(LOCAL_PATH)/../framework/common
include $(VAL_HWC_HWC_COMMON_INC_PATH)/Hwcval.mk

LOCAL_SRC_FILES:=\
    surfaceflingershim.cpp

LOCAL_MODULE:=valhwc_surfaceflingershim
LOCAL_MODULE_PATH=$(VAL_HWC_TARGET_TEST_PATH)/bin

LOCAL_SHARED_LIBRARIES += liblog

include $(BUILD_EXECUTABLE)
