LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

## Manage software and hardware variants.
VAL_HWC_HWC_COMMON_INC_PATH:=$(LOCAL_PATH)/../common
include $(VAL_HWC_HWC_COMMON_INC_PATH)/Hwcval.mk

ifndef VAL_HWC_TOP
$(error VAL_HWC_TOP not defined)
endif

VAL_HWC_DOXYGEN_SRC_DIR := ""

LOCAL_SRC_FILES := \
    iVP_shim.cpp

LOCAL_SHARED_LIBRARIES += \
    libgrallocclient \
    libdl \
    libcutils \
    libutils \
    libvalhwccommon \
    liblog

LOCAL_STATIC_LIBRARIES +=

LOCAL_CFLAGS += -DHWCVAL_LOG_$(HWCVAL_LOG_VERBOSITY) -DHWCVAL_LOG_$(HWCVAL_LOG_DESTINATION) \
    -rdynamic -fvisibility=default

LOCAL_C_INCLUDES += \
    $(VAL_HWC_HWC_COMMON_INC_PATH) \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/../hwc_shim/ \
    $(VAL_HWC_HARDWARE_COMPOSER_PATH)/libhwcservice \
    $(VAL_HWC_LIB_IVP_PATHS)

LOCAL_LDLIBS += -ldl

LOCAL_MODULE_TAGS:= optional
LOCAL_MODULE:= libvalhwc_ivpshim
include $(VAL_HWC_TOP)/common/ModuleCommon.mk

include $(BUILD_SHARED_LIBRARY)

