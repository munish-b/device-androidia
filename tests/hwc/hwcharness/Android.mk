LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

## Manage software and hardware variants.
VAL_HWC_HWC_COMMON_INC_PATH:=$(LOCAL_PATH)/../framework/common
include $(VAL_HWC_HWC_COMMON_INC_PATH)/Hwcval.mk

ifndef VAL_HWC_TOP
$(error VAL_HWC_TOP not defined)
endif

VAL_HWC_DOXYGEN_SRC_DIR := ""

VAL_HWC_FEATURE := hwc

VAL_HWC_MDSSHIM_INC_PATH=$(LOCAL_PATH)/../framework/mds_shim
VAL_HWC_WIDISHIM_INC_PATH=$(LOCAL_PATH)/../framework/widi_shim
VAL_HWC_LIBPNG_INC_PATH=$(ANDROID_BUILD_TOP)/external/libpng
VAL_HWC_ZLIB_INC_PATH=$(ANDROID_BUILD_TOP)/external/zlib

LOCAL_SRC_FILES:=\
    HwchInterface.cpp \
    HwchGlInterface.cpp \
    HwcHarness.cpp \
    HwchChoice.cpp \
    HwchDisplayChoice.cpp \
    HwchLayerChoice.cpp \
    HwchTest.cpp \
    HwchTests.cpp \
    HwchRandomTest.cpp \
    HwchApiTest.cpp \
    HwchRandomModesTest.cpp \
    HwchDirectPlanesTest.cpp \
    HwchProtectedContentTests.cpp \
    HwchFlickerTests.cpp \
    HwchStressTests.cpp \
    HwchModeTests.cpp \
    HwchReplayParser.cpp \
    HwchReplayRunner.cpp \
    HwchReplayHWCLRunner.cpp \
    HwchReplayDSRunner.cpp \
    HwchReplayDSLayers.cpp \
    HwchLayer.cpp \
    HwchLayers.cpp \
    HwchFrame.cpp \
    HwchDisplay.cpp \
    HwchSystem.cpp \
    HwchAbstractPavpSession.cpp \
    HwchPavpSession.cpp \
    HwchFakePavpSession.cpp \
    HwchPattern.cpp \
    HwchGlPattern.cpp \
    HwchPatternMgr.cpp \
    HwchBufferSet.cpp \
    HwchBufferDestroyer.cpp \
    HwchTimelineThread.cpp \
    HwchVSync.cpp \
    HwchBufferFormatConfig.cpp \
    HwchDisplaySpoof.cpp \
    HwchPngImage.cpp \
    HwchWatchdogThread.cpp \
    HwchAsyncEventGenerator.cpp \
    HwchInputGenerator.cpp \
    HwchRange.cpp

ifdef HWCVAL_BUILD_INTERNAL_TESTS
LOCAL_SRC_FILES += HwchInternalTests.cpp
endif

ifdef HWCVAL_BUILD_GL_TESTS
LOCAL_SRC_FILES += HwchGlTests.cpp
endif
   
LOCAL_C_INCLUDES += $(VAL_HWC_TOP)/tests/hwc/intel 
#if 0
ifeq ($(BUILD_PAVP),1)
    ifeq ($(VAL_HWC_HAVE_UFO), 1)
        LOCAL_C_INCLUDES += hardware/intel/ufo/ufo/Source/Android/cp/libpavp/src \
                            hardware/intel/ufo/ufo/Source/Android/core/coreu/include \
                            hardware/intel/ufo/ufo/Source/Android/core/coreu/src/libcoreu/libcoreuservice/pavp/include \
                            hardware/intel/ufo/ufo/Source/Android/include \
                            hardware/intel/ufo/ufo/Source/inc/common \
                            hardware/intel/ufo/ufo/Source/GmmLib/inc
    else
        LOCAL_C_INCLUDES += $(LOCAL_PATH)/PAVP
    endif

    LOCAL_SHARED_LIBRARIES += libpavp
    LOCAL_SHARED_LIBRARIES += libcoreuinterface libcoreuservice
endif
#endif
LOCAL_SHARED_LIBRARIES += \
    libbinder \
    libdrm \
    libdrm_intel \
    libdl \
    libcutils \
    libutils \
    libhardware \
    libhwcservice \
    libui \
    libvalhwccommon \
    libsync \
    libEGL \
    libGLESv2 \
    libpng \
    liblog

ifneq ($(HWCVAL_RESOURCE_LEAK_CHECKING),)
LOCAL_SHARED_LIBRARIES += \
    libc_malloc_debug_leak
endif

# Include Widi support if Intel Widi is available on this platform
ifeq ($(INTEL_WIDI), true)
    LOCAL_SRC_FILES += \
        HwchWidi.cpp \
        HwchWidiReleaseFencePool.cpp \
        HwchWidiFrameListener.cpp \
        HwchWidiFrameTypeChangeListener.cpp \
        HwchLayerWindowed.cpp \
        HwchWidiTests.cpp

    LOCAL_SHARED_LIBRARIES += libhwcwidi libEGL
endif

LOCAL_MODULE:= valhwcharness
LOCAL_MODULE_PATH=$(VAL_HWC_TARGET_TEST_PATH)/bin

include $(BUILD_EXECUTABLE)
