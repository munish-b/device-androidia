
# Determine the DRM include path
ifeq ($(VAL_HWC_LIB_DRM_PATHS),)
    ifeq (,$(wildcard $(ANDROID_PRODUCT_OUT)/obj/SHARED_LIBRARIES/libdrm_intermediates/export_includes ))
        $(error libdrm must be build first)
    else
        awk_command_drm=awk '{print $$2}' $(ANDROID_PRODUCT_OUT)/obj/SHARED_LIBRARIES/libdrm_intermediates/export_includes
        VAL_HWC_LIB_DRM_PATHS=$(shell $(awk_command_drm))
    endif
endif

# Determine the iVP include path
ifeq ($(VAL_HWC_LIB_IVP_PATHS),y)
    ifeq (,$(wildcard $(ANDROID_PRODUCT_OUT)/obj/SHARED_LIBRARIES/libivp_intermediates/export_includes ))
        $(error libivp must be build first)
    else
        awk_command_ivp=awk '{print $$2}' $(ANDROID_PRODUCT_OUT)/obj/SHARED_LIBRARIES/libivp_intermediates/export_includes
        VAL_HWC_LIB_IVP_PATHS=$(shell $(awk_command_ivp))
    endif
endif

ifeq ($(VAL_HWC_HARDWARE_COMPOSER_PATH),)
    ifeq (,$(wildcard $(ANDROID_PRODUCT_OUT)/obj/SHARED_LIBRARIES/libhwcservice_intermediates/export_includes))
        $(error HWC service must be built first)
    else
        awk_command_1=awk '{print $$2}' $(ANDROID_PRODUCT_OUT)/obj/SHARED_LIBRARIES/libhwcservice_intermediates/export_includes | xargs dirname
        VAL_HWC_HARDWARE_COMPOSER_PATH=$(shell $(awk_command_1))
    endif
endif

VAL_HWC_HARDWARE_COMPOSER_PATH?=$(ANDROID_BUILD_TOP)/vendor/intel/external/android_ia/hwcomposer
ifeq ($(VAL_HWC_HARDWARE_COMPOSER_PATH_PRINTED),)
    $(info VAL_HWC_HARDWARE_COMPOSER_PATH $(VAL_HWC_HARDWARE_COMPOSER_PATH))
    VAL_HWC_HARDWARE_COMPOSER_PATH_PRINTED=Y
endif

VAL_HWC_HWCSERVICE_INC_PATH=$(VAL_HWC_HARDWARE_COMPOSER_PATH)/os/android/libhwcservice
VAL_HWC_LIBHW_INC_PATH=hardware/libhardware/include/hardware
VAL_HWC_LIBMULTIDISPLAY_INC_PATH=vendor/intel/hardware/libmultidisplay/native/include
VAL_HWC_LIBMULTIDISPLAY_INC_PATH += vendor/intel/hardware/libmedia_utils/libmultidisplay/native/include
VAL_HWC_LIB_STL := external/stlport/stlport

# Path for binaries
ifeq ($(VAL_HWC_EXTERNAL_BUILD),)
    VAL_HWC_TARGET_TEST_PATH := $(TARGET_OUT_VENDOR)
endif

VAL_HWC_FEATURE := hwc

ifeq ($(strip $(HWCVAL_ENABLE_SYSTRACE)),true)
LOCAL_CFLAGS += \
    -DHWCVAL_SYSTRACE=1 \
    -DHAVE_ANDROID_OS
endif

LOCAL_CFLAGS += \
    -DHWCVAL_LOG_$(HWCVAL_LOG_VERBOSITY) \
    -DHWCVAL_LOG_$(HWCVAL_LOG_DESTINATION)

# Define whether or not we are compiling for Broxton
ifneq (,$(filter bxt_rvp bxtp_abl,$(TARGET_PRODUCT)))
    LOCAL_CFLAGS += -DHWCVAL_BROXTON
endif

ifneq ($(wildcard system/core/libsync/sw_sync.h),)
    LOCAL_CFLAGS += -DSW_SYNC_H_PATH="<sync/../../sw_sync.h>"
else
    LOCAL_CFLAGS += -DSW_SYNC_H_PATH="<sync/sw_sync.h>"
endif

# From UFO CL 280279 onwards, the field naming in the intel_ufo_buffer_details_t
# structure has changed.
LOCAL_CFLAGS += -DHWCVAL_FB_BUFFERINFO_FORMAT

ifneq ($(HWCVAL_INTERNAL_BO_VALIDATION),)
    LOCAL_CFLAGS += -DHWCVAL_INTERNAL_BO_VALIDATION=$(HWCVAL_INTERNAL_BO_VALIDATION)
endif

LOCAL_C_INCLUDES += $(VAL_HWC_HARDWARE_COMPOSER_PATH)
LOCAL_C_INCLUDES += $(VAL_HWC_HARDWARE_COMPOSER_PATH)/common/utils
LOCAL_C_INCLUDES += $(VAL_HWC_HARDWARE_COMPOSER_PATH)/val
LOCAL_C_INCLUDES += $(VAL_HWC_HARDWARE_COMPOSER_PATH)/wsi
LOCAL_C_INCLUDES += $(VAL_HWC_HARDWARE_COMPOSER_PATH)/wsi/drm
LOCAL_C_INCLUDES += $(VAL_HWC_HARDWARE_COMPOSER_PATH)/os
LOCAL_C_INCLUDES += $(VAL_HWC_HARDWARE_COMPOSER_PATH)/os/android
LOCAL_C_INCLUDES += $(VAL_HWC_HARDWARE_COMPOSER_PATH)/public
LOCAL_C_INCLUDES += $(VAL_HWC_HWCSERVICE_INC_PATH)
LOCAL_C_INCLUDES += $(ANDROID_BUILD_TOP)/external/minigbm
LOCAL_C_INCLUDES += $(ANDROID_BUILD_TOP)/external/minigbm/cros_gralloc

BUILD_SHIM_HWCSERVICE := 0
BUILD_HWCSERVICE_API := 1

# Not currently building PAVP
BUILD_PAVP := 0

ifneq ($(shell grep -r drm_i915_disp_screen_control ${VAL_HWC_LIB_DRM_PATHS}),)
    LOCAL_CFLAGS += -DBUILD_I915_DISP_SCREEN_CONTROL
endif

ifeq ($(BUILD_PAVP),1)
    LOCAL_CFLAGS += -DHWCVAL_BUILD_PAVP
endif

ifeq (X$(HWCVAL_NO_FRAME_CONTROL),X)
    LOCAL_CFLAGS += -DHWCVAL_FRAME_CONTROL=1
endif

ifneq ($(HWCVAL_RESOURCE_LEAK_CHECKING),)
    LOCAL_CFLAGS += -DHWCVAL_RESOURCE_LEAK_CHECKING
endif

ifeq ($(strip $(BOARD_USES_GRALLOC1)), true)
LOCAL_CFLAGS += -DHWCVAL_ENABLE_GRALLOC1
LOCAL_CFLAGS += -DUSE_MINIGBM
endif

# Enable private interfaces in HWC include files
LOCAL_CFLAGS += -DINTEL_HWC_INTERNAL_BUILD

# Enable ALOG_ASSERT
LOCAL_CFLAGS += -DLOG_NDEBUG=0

# Enable C++11 features
LOCAL_CFLAGS += -std=gnu++11

ifeq ($(VAL_HWC_EXTERNAL_BUILD),)
    LOCAL_STRIP_MODULE := false
endif

# No optimize, for debugging
#LOCAL_CFLAGS += -O0

LOCAL_CFLAGS += -DANDROID_VERSION=$(major)$(minor)$(rev)

LOCAL_C_INCLUDES += $(VAL_HWC_LIB_DRM_PATHS)


# M_Dessert+ - condition
ifeq ($(shell test $(major) -lt 6; echo $$?), 0)
    # Up to L
    include $(VAL_HWC_LIB_STL)/../libstlport.mk
    LOCAL_STATIC_LIBRARIES += libregex-re2

    # Path of the external regex library
    LOCAL_C_INCLUDES += external/regex-re2/
else
    # M+
    LOCAL_STATIC_LIBRARIES += libregex-re2-tmp-gnustl-rtti

    # Use CLANG on M+
    LOCAL_CLANG := true
endif

# Version number stuff
HWC_VERSION_GIT_BRANCH := $(shell pushd $(VAL_HWC_HARDWARE_COMPOSER_PATH) > /dev/null; git rev-parse --abbrev-ref HEAD; popd > /dev/null)
HWC_VERSION_GIT_SHA := $(shell pushd $(VAL_HWC_HARDWARE_COMPOSER_PATH) > /dev/null; git rev-parse HEAD; popd > /dev/null)
HWCVAL_VERSION_GIT_BRANCH := $(shell pushd $(HWCVAL_ROOT) > /dev/null; git rev-parse --abbrev-ref HEAD; popd > /dev/null)
HWCVAL_VERSION_GIT_SHA := $(shell pushd $(HWCVAL_ROOT) > /dev/null; git rev-parse HEAD; popd > /dev/null)
LOCAL_CFLAGS += -DHWC_VERSION_GIT_BRANCH=$(HWC_VERSION_GIT_BRANCH) -DHWC_VERSION_GIT_SHA=$(HWC_VERSION_GIT_SHA) \
  -DHWCVAL_VERSION_GIT_BRANCH=$(HWCVAL_VERSION_GIT_BRANCH) -DHWCVAL_VERSION_GIT_SHA=$(HWCVAL_VERSION_GIT_SHA)

LOCAL_CFLAGS += -Wall

