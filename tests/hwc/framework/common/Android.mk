
VAL_HWC_SRC_PATH:= $(call my-dir)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

## Manage software and hardware variants.
include $(LOCAL_PATH)/Hwcval.mk

VAL_HWC_HWC_COMMON_INC_PATH=$(LOCAL_PATH)

ifndef VAL_HWC_TOP
$(error VAL_HWC_TOP not defined)
endif

VAL_HWC_DOXYGEN_SRC_DIR := ""

# Build the static library
LOCAL_SRC_FILES:=\
    HwcTestConfig.cpp \

LOCAL_CFLAGS += -fvisibility=default -DHWCVAL_LOG_$(HWCVAL_LOG_VERBOSITY) \
    -DHWCVAL_LOG_$(HWCVAL_LOG_DESTINATION)

LOCAL_C_INCLUDES += \
    $(VAL_HWC_HWC_COMMON_INC_PATH) \
    $(VAL_HWC_SRC_PATH) \
    $(LOCAL_PATH)

LOCAL_SHARED_LIBRARIES += libbinder
LOCAL_MODULE_TAGS:= optional
LOCAL_MODULE:= libvalhwcstatic
include $(VAL_HWC_TOP)/common/ModuleCommon.mk

include $(BUILD_STATIC_LIBRARY)

# Now build the shared library
include $(CLEAR_VARS)

## Manage software and hardware variants.
include $(LOCAL_PATH)/Hwcval.mk

LOCAL_SRC_FILES:=\
    hwc_shim_binder.cpp \
    HwcTestConfig.cpp \
    HwcCrcReader.cpp \
    HwcTestCrtc.cpp \
    HwcTestLog.cpp \
    HwcTestState.cpp \
    HwcvalContent.cpp \
    HwcvalHwc1Content.cpp \
    HwcvalHwc1.cpp \
    HwcTestReferenceComposer.cpp \
    HwcTestCompValThread.cpp \
    HwcTestKernel.cpp \
    HwcTestProtectionChecker.cpp \
    HwcTestTimeline.cpp \
    HwcTestUtil.cpp \
    HwcTestDebug.cpp \
    HwcTestDisplaySpoof.cpp \
    HwcvalWork.cpp \
    HwcvalWatchdog.cpp \
    HwcvalStall.cpp \
    HwcvalStatistics.cpp \
    HwcvalThreadTable.cpp \
    HwcvalLogParser.cpp \
    HwcvalDrmParser.cpp \
    HwcvalLogDisplay.cpp \
    HwcvalLayerListQueue.cpp \
    DrmShimChecks.cpp \
    DrmShimCrtc.cpp \
    DrmShimPlane.cpp \
    DrmShimBuffer.cpp \
    BufferObject.cpp \
    DrmShimTransform.cpp \
    DrmShimCallbackBase.cpp \
    DrmShimWork.cpp \
    SSIMUtils.cpp \
    CrcDebugfs.cpp \
    HwcvalDebug.cpp

LOCAL_SRC_FILES += HwcvalServiceManager.cpp

ifeq ($(BUILD_SHIM_HWCSERVICE),1)
    LOCAL_SRC_FILES += \
        BxService.cpp  \
        HwcServiceShim.cpp \
        HwcTestMdsControl.cpp \
        HwcTestDisplayControl.cpp
endif

LOCAL_SHARED_LIBRARIES += libgrallocclient \
    libdl \
    libhardware \
    libhwcservice \
    libbinder \
    libutils \
    libcutils \
    libsync \
    libui \
    libGLESv1_CM \
    libGLESv2 \
    libEGL \
    liblog

LOCAL_C_INCLUDES += \
    $(VAL_HWC_HWC_COMMON_INC_PATH) \
    $(VAL_HWC_SRC_PATH) \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/../drm_shim/ \
    $(LOCAL_PATH)/../mds_shim/ \
    $(VAL_HWC_LIB_DRM_PATHS) \
    $(VAL_HWC_LIB_IVP_PATHS) \
    $(VAL_HWC_HARDWARE_COMPOSER_PATH)/libhwcservice \
    $(VAL_HWC_MDS_PATH)

LOCAL_EXPORT_C_INCLUDE_DIRS += $(LOCAL_PATH)
LOCAL_MODULE_TAGS:= optional
LOCAL_MODULE:= libvalhwccommon
include $(VAL_HWC_TOP)/common/ModuleCommon.mk

include $(BUILD_SHARED_LIBRARY)

