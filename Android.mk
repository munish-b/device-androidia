
# This is a very top level make file to build the HWC test framework and tests.
# This is intended to operate from the Android build.

HWC_VAL_TEST:= y

ifneq (,$(filter $(HWC_VAL_TEST), Y y YES yes))

    VAL_HWC_TOP:= $(call my-dir)
    HWCVAL_ROOT:=$(VAL_HWC_TOP)/tests/hwc
    VAL_HWC_HARDWARE_COMPOSER_PATH:=$(ANDROID_BUILD_TOP)/vendor/intel/external/android_ia/hwcomposer
    VAL_HWC_TESTS_PATH:=$(VAL_HWC_HARDWARE_COMPOSER_PATH)/common/utils
    LOCAL_MODULE:=val_hwc

    VAL_HWC_EXTERNAL_BUILD = true
    VAL_HWC_TARGET_TEST_PATH := $(ANDROID_PRODUCT_OUT)/$(TARGET_COPY_OUT_VENDOR)/intel/validation/val_hwc
    $(info VAL_HWC_TARGET_TEST_PATH $(VAL_HWC_TARGET_TEST_PATH))

    # hwclogviewer
    include $(CLEAR_VARS)
    LOCAL_MODULE:= hwclogviewer
    LOCAL_PATH:=$(VAL_HWC_TESTS_PATH)
    LOCAL_SRC_FILES:= LogView.cpp
    include $(VAL_HWC_HARDWARE_COMPOSER_PATH)/Android.common.mk
    ifeq ($(strip $(INTEL_HWC_LOGVIEWER_BUILD)),true)
	include $(BUILD_EXECUTABLE)
    endif


    include $(CLEAR_VARS)
    LOCAL_MODULE:=val_hwc
    LOCAL_ADDITIONAL_DEPENDENCIES := val_hwc_tar val_hwc_modules
    include $(BUILD_PHONY_PACKAGE)

    # HWC validation
    include $(HWCVAL_ROOT)/Android.mk

    val_hwc: val_hwc_tar

    include $(CLEAR_VARS)
    LOCAL_MODULE:=val_hwc_modules
    LOCAL_MODULE_PATH:=$(VAL_HWC_TARGET_TEST_PATH)
    include $(BUILD_PHONY_PACKAGE)

    val_hwc_modules: \
	libvalhwccommon \
	libvalhwc_drmshim \
	valhwc_composershim \
	valhwcharness \
	hwclogviewer

    # Add Multi-display service, if needed
    ifeq ($(strip $(TARGET_HAS_MULTIPLE_DISPLAY)), true)
    val_hwc_modules: \
	libvalhwc_mdsshim
    endif

    # Compile in the widi components if needed
    ifneq ($(filter true, $(INTEL_WIDI_BAYTRAIL) $(INTEL_WIDI_GEN)),)
    val_hwc_modules: \
	libvalhwc_widishim
    endif

    # Files names are of a format requested by the QB team to be consistent with
    # other artifact names.
    # Postfix for use in the artifact out directory required by QB
    VAL_HWC_TARGET_ARCH_DIR_POSTFIX:=32
    ifeq ($(TARGET_ARCH),x86_64)
    VAL_HWC_TARGET_ARCH_DIR_POSTFIX:=64
    else ifneq ($(TARGET_ARCH),x86)
    # warning only so we do not break the GFT builds
    $(warning HWC Validation: unknown TARGET_ARCH, not x86 or x86_64. Default to 32-bit output path postfix)
    endif

    VAL_HWC_BUILD_POSTFIX_RELEASE:=R
    VAL_HWC_BUILD_POSTFIX_RELEASE_INTERNAL:=RI
    VAL_HWC_BUILD_POSTFIX_DEBUG:=D

    # QB passes BUILD_TYPE with camel case and the UFO build uses lower case
    ifeq ($(BUILD_TYPE),Release)
    VAL_HWC_BUILD_POSTFIX:=$(VAL_HWC_BUILD_POSTFIX_RELEASE)
    else ifeq ($(BUILD_TYPE),release)
    VAL_HWC_BUILD_POSTFIX:=$(VAL_HWC_BUILD_POSTFIX_RELEASE)
    else ifeq ($(BUILD_TYPE),Release-Internal)
    VAL_HWC_BUILD_POSTFIX:=$(VAL_HWC_BUILD_POSTFIX_RELEASE_INTERNAL)
    else ifeq ($(BUILD_TYPE),release-internal)
    VAL_HWC_BUILD_POSTFIX:=$(VAL_HWC_BUILD_POSTFIX_RELEASE_INTERNAL)
    else ifeq ($(BUILD_TYPE),Debug)
    VAL_HWC_BUILD_POSTFIX:=$(VAL_HWC_BUILD_POSTFIX_DEBUG)
    else ifeq ($(BUILD_TYPE),debug)
    VAL_HWC_BUILD_POSTFIX:=$(VAL_HWC_BUILD_POSTFIX_DEBUG)
    else
    VAL_HWC_BUILD_POSTFIX:=local_build
    endif

    ifeq (,$(ARTIFACT_DESTINATION))
	VAL_HWC_QB_OUT_PRODUCT_PATH := $(VAL_HWC_QB_OUT_PATH)$(VAL_HWC_TARGET_ARCH_DIR_POSTFIX)/$(TARGET_PRODUCT)
	VAL_HWC_ARTIFACT_NAME := $(TARGET_PRODUCT)-val_hwc-$(UFO_VERSION)-$(VAL_HWC_BUILD_POSTFIX).tar.gz
    else
	VAL_HWC_QB_OUT_PRODUCT_PATH := $(ARTIFACT_DESTINATION)
	VAL_HWC_ARTIFACT_NAME := val_hwc_$(UFO_VERSION)_$(VAL_HWC_BUILD_POSTFIX)_$(TARGET_PRODUCT)_$(VAL_HWC_TARGET_ARCH_DIR_POSTFIX).tgz
    endif

    val_hwc_tar: val_hwc_modules
	    #
	    # Copy hwclogviewer into the bin directory
	    # (HWC makefiles build into main bin directory).
	    mkdir -p $(VAL_HWC_TARGET_TEST_PATH)/
	    mkdir -p $(VAL_HWC_TARGET_TEST_PATH)/bin
	    if [ -e ${ANDROID_PRODUCT_OUT}/$(TARGET_COPY_OUT_VENDOR)/bin/hwclogviewer ]; \
	    then \
		mv $(ANDROID_PRODUCT_OUT)/$(TARGET_COPY_OUT_VENDOR)/bin/hwclogviewer $(VAL_HWC_TARGET_TEST_PATH)/bin; \
	    else \
		mv $(ANDROID_PRODUCT_OUT)/system/bin/hwclogviewer $(VAL_HWC_TARGET_TEST_PATH)/bin; \
	    fi
	    #
	    # Create target scripts directory
	    rm -rf $(VAL_HWC_TARGET_TEST_PATH)/client_scripts
	    mkdir -p $(VAL_HWC_TARGET_TEST_PATH)
	    cp -rf $(HWCVAL_ROOT)/client_scripts $(VAL_HWC_TARGET_TEST_PATH)
	    chmod a+x $(VAL_HWC_TARGET_TEST_PATH)/client_scripts/*
	    #
	    # Create host scripts directory
	    cp -rf $(HWCVAL_ROOT)/host_scripts $(VAL_HWC_TARGET_TEST_PATH)
	    #
	    # Add the images
	    cp -rf $(HWCVAL_ROOT)/images $(VAL_HWC_TARGET_TEST_PATH)
	    #
	    # Tar up the result
	    mkdir -p $(VAL_HWC_QB_OUT_PRODUCT_PATH)
	    tar -czf $(VAL_HWC_QB_OUT_PRODUCT_PATH)/$(VAL_HWC_ARTIFACT_NAME) \
		    -C $(VAL_HWC_TARGET_TEST_PATH)/../ val_hwc

endif

