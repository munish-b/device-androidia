LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

## Manage software and hardware variants.
include $(VAL_HWC_HWC_COMMON_INC_PATH)/Hwcval.mk

LOCAL_SRC_FILES:=\
    kmsgReader.cpp

LOCAL_MODULE:=valhwc_kmsgReader
LOCAL_MODULE_PATH=$(VAL_HWC_TARGET_TEST_PATH)/bin
    
include $(BUILD_EXECUTABLE)
