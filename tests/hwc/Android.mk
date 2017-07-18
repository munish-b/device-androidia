
# This is a top level makefile to build the HWC test framework and related tests.
# Building from this level will build everything needed to run the tests.

HWCVAL_TOP_SRC_PATH:= $(call my-dir)

ifeq ($(VAL_HWC_EXTERNAL_BUILD),)
    VAL_HWC_TARGET_TEST_PATH := $(TARGET_OUT_VENDOR)

    # Temporary workaround to be used until regex-re2 bugs are fixed.
    include $(HWCVAL_TOP_SRC_PATH)/framework/common/DetermineAndroidVersion.mk
    ifeq ($(shell test $(major) -ge 6; echo $$?), 0)
        include $(HWCVAL_TOP_SRC_PATH)/../../regex-re2_temp/Android.mk
    else
        include external/regex-re2/Android.mk
    endif
else
    # Temporary workaround to be used until regex-re2 bugs are fixed.
    include $(HWCVAL_TOP_SRC_PATH)/framework/common/DetermineAndroidVersion.mk
    ifeq ($(shell test $(major) -ge 6; echo $$?), 0)
        include $(HWCVAL_TOP_SRC_PATH)/../../regex-re2_temp/Android.mk
    endif
endif

include $(HWCVAL_TOP_SRC_PATH)/framework/common/Android.mk
include $(HWCVAL_TOP_SRC_PATH)/framework/drm_shim/Android.mk
include $(HWCVAL_TOP_SRC_PATH)/framework/hwc_shim/Android.mk

include $(HWCVAL_TOP_SRC_PATH)/tests/Android.mk

include $(HWCVAL_TOP_SRC_PATH)/hwcharness/Android.mk
include $(HWCVAL_TOP_SRC_PATH)/surfaceflingershim/Android.mk
include $(HWCVAL_TOP_SRC_PATH)/tools/kmsgReader/Android.mk
