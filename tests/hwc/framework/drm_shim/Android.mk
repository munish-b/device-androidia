LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

## Manage software and hardware variants.
VAL_HWC_HWC_COMMON_INC_PATH:=$(LOCAL_PATH)/../common
include $(VAL_HWC_HWC_COMMON_INC_PATH)/Hwcval.mk

ifndef VAL_HWC_TOP
$(error VAL_HWC_TOP not defined)
endif

VAL_HWC_DOXYGEN_SRC_DIR := ""

VAL_HWC_FEATURE := hwc

LOCAL_SRC_FILES := \
    drm_shim.cpp \
    DrmShimEventHandler.cpp \
    DrmShimPropertyManager.cpp

LOCAL_SHARED_LIBRARIES += \
    libdl \
    libcutils \
    libutils \
    libhardware \
    libvalhwccommon \
    liblog

LOCAL_STATIC_LIBRARIES +=

LOCAL_C_INCLUDES += \
    $(VAL_HWC_HWC_COMMON_INC_PATH) \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/../hwc_shim/ \
    $(VAL_HWC_TOP)/tests/hwc/hwcharness \
    $(VAL_HWC_TOP)/../../../libdrm/intel/ \
    $(VAL_HWC_TOP)/libhwcservice \
    $(VAL_HWC_TOP)/../../common/utils/val

LOCAL_LDLIBS += -ldl

LOCAL_MODULE_TAGS:= optional
LOCAL_MODULE:= libvalhwc_drmshim
include $(VAL_HWC_TOP)/common/ModuleCommon.mk

include $(BUILD_SHARED_LIBRARY)

