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
    MultiDisplayShimService.cpp

LOCAL_MODULE:= libvalhwc_mdsshim
LOCAL_MODULE_TAGS := optional
include $(VAL_HWC_TOP)/common/ModuleCommon.mk

LOCAL_SHARED_LIBRARIES += \
    libgrallocclient \
    libbinder \
    libivp \
    libdl \
    libui \
    libcutils \
    libutils \
    libhardware \
    libmultidisplay \
    libhwcservice \
    libvalhwccommon

LOCAL_CFLAGS += -DLOG_TAG=\"MultiDisplayShim\"

LOCAL_C_INCLUDES += \
    $(VAL_HWC_HWC_COMMON_INC_PATH) \
    $(VAL_HWC_LIBMULTIDISPLAY_INC_PATH)

ifeq ($(TARGET_HAS_VPP),true)
LOCAL_CFLAGS += -DTARGET_HAS_VPP
LOCAL_C_INCLUDES += $(TARGET_OUT_HEADERS)/libmedia_utils_vpp
LOCAL_SHARED_LIBRARIES += libvpp_setting
endif

# Check if getVppState function exists in the interface
ifneq ($(shell grep -r getVppState $(wildcard $(VAL_HWC_LIBMULTIDISPLAY_INC_PATH))),)
LOCAL_CFLAGS += -DHWCVAL_GETVPPSTATE_EXISTS
endif

include $(BUILD_SHARED_LIBRARY)

