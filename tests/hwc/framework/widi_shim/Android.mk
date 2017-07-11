VAL_HWC_SRC_PATH:= $(call my-dir)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifndef VAL_HWC_TOP
$(error VAL_HWC_TOP not defined)
endif

## Manage software and hardware variants.
VAL_HWC_HWC_COMMON_INC_PATH:=$(LOCAL_PATH)/../common
include $(VAL_HWC_HWC_COMMON_INC_PATH)/Hwcval.mk

VAL_HWC_DOXYGEN_SRC_DIR := ""

LOCAL_SRC_FILES:= \
    WidiShimService.cpp \
    WidiShimFrameListener.cpp \
    WidiShimFrameTypeChangeListener.cpp

LOCAL_MODULE:= libvalhwc_widishim
LOCAL_MODULE_TAGS := optional
include $(VAL_HWC_TOP)/common/ModuleCommon.mk

LOCAL_SHARED_LIBRARIES := \
    libgrallocclient \
    libivp \
    libhwcwidi libutils libcutils libbinder libvalhwccommon libsync

LOCAL_CFLAGS += -DLOG_TAG=\"WidiShim\"

LOCAL_C_INCLUDES += \
    $(VAL_HWC_HWC_COMMON_INC_PATH) \
    $(VAL_HWC_HARDWARE_COMPOSER_PATH)/common \
    $(VAL_HWC_HARDWARE_COMPOSER_PATH)/libhwcservice

include $(BUILD_SHARED_LIBRARY)
