
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

LOCAL_SRC_FILES:=\
    hwc_shim.cpp \
    HwcTimeline.cpp \
    HwcDrmShimCallback.cpp

LOCAL_CFLAGS += -rdynamic -O0  -DHWCVAL_LOG_$(HWCVAL_LOG_VERBOSITY) \
    -DHWCVAL_LOG_$(HWCVAL_LOG_DESTINATION) \

LOCAL_C_INCLUDES += \
    $(VAL_HWC_HWC_COMMON_INC_PATH) \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/../common/ \
    $(LOCAL_PATH)/../drm_shim/ \
    $(LOCAL_PATH)/../mds_shim/

# Compile in the widi components if needed
ifneq ($(filter true, $(INTEL_WIDI_BAYTRAIL) $(INTEL_WIDI_GEN)),)
    LOCAL_CFLAGS += -DTARGET_HAS_MCG_WIDI=1
    LOCAL_C_INCLUDES += $(LOCAL_PATH)/../widi_shim/
    LOCAL_SHARED_LIBRARIES += libvalhwc_widishim
endif

LOCAL_SHARED_LIBRARIES += \
    libgrallocclient \
    libivp \
    libdrm \
    libdrm_intel \
    libdl \
    libcutils \
    libutils \
    libhardware \
    libhwcservice \
    libvalhwccommon \
    libbinder \
    libsync \
    liblog

# Compile in the MCG Multi display components if needed
ifeq ($(strip $(TARGET_HAS_MULTIPLE_DISPLAY)), true)
    LOCAL_SHARED_LIBRARIES += libvalhwc_mdsshim
endif

LOCAL_STATIC_LIBRARIES +=

LOCAL_MODULE_TAGS:= optional
LOCAL_MODULE:= valhwc_composershim
include $(VAL_HWC_TOP)/common/ModuleCommon.mk

include $(BUILD_SHARED_LIBRARY)

