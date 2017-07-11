LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:=\
    gttmem.c

LOCAL_SHARED_LIBRARIES += \
    libpciaccess

#LOCAL_STATIC_LIBRARIES

LOCAL_MODULE_TAGS:= optional
LOCAL_MODULE:= gttmem
include $(VAL_HWC_TOP)/common/ModuleCommon.mk

include $(BUILD_EXECUTABLE)


