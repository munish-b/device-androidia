/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __HwcTestState_h__
#define __HwcTestState_h__

#include <dlfcn.h>
#include <cutils/log.h>
#include <assert.h>

#include "intel_bufmgr.h"

extern "C"
{
#include "hardware/hardware.h"
#include "hardware/hwcomposer2.h"
#include <hardware/gralloc.h>
}

#include <utils/Vector.h>
#include <utils/SystemClock.h>
#include <binder/IServiceManager.h>
#include <utils/Condition.h>

#include "HwcvalDebug.h"
#include "HwcTestDefs.h"
#include "HwcTestConfig.h"
#include "HwcTestLog.h"
#include "HwcTestDisplaySpoof.h"
#include "HwcvalStall.h"

class HwcShimService;
class HwcShimInitializer;
class HwcTestKernel;
class DrmShimChecks;
class HwcShim;
class HwcServiceShim;

namespace Hwcval
{
    class MultiDisplayInfoProviderShim;
    class Selector;
}

android::sp<android::IServiceManager> realServiceManager();

class HwcTestEventHandler
{
    public:
        virtual ~HwcTestEventHandler()
        {
        }

        virtual void CaptureVBlank(int fd, uint32_t crtcId) = 0;

        virtual void Restore(uint32_t crtcId) = 0;

        virtual void CancelEvent(uint32_t crtcId) = 0;
};


class HwcTestState
{
    public:
        // Device attribute query
        enum DeviceType
        {
            eDeviceTypeBYT = 0,
            eDeviceTypeCHT,
            eDeviceTypeBXT,
            eDeviceTypeUnknown
        };

        enum DisplayType
        {
            eFixed = 1,
            eRemovable = 2,
            eVirtual = 4
        };

        // Display attribute query
        enum DisplayPropertyType
        {
            ePropNone = 0,
            ePropConnectorId
        };

        enum ShimMaskType
        {
            eHwcShim = 1,
            eDrmShim = 2,
            eIvpShim = 4,
            eMdsShim = 8
        };

        // Typedef for HWC Log function pointer
        typedef void (*HwcLogAddPtr)(const char* fmt, ...);

        // Typedef for hotplug simulate function
        typedef void (*HwcSimulateHotPlugPtr)(bool connected);

        //typedefs for DRM functions we need to be given pointers for
        typedef int (*FPdrm_intel_bo_map) (::drm_intel_bo *bo, int write_enable);
        typedef int (*FPdrm_intel_bo_unmap)(::drm_intel_bo *bo);

    protected:
        /// Get display properties
        void SetupDisplayProperties(void);
        /// TODO
        void SetupCheckFunctions(uint32_t platform);

        // Check functions set error of the shim internally and so do not return a
        // error code.

#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE
        // Shim of HWC IService
        android::sp<HwcServiceShim> mHwcServiceShim;
#endif // HWCVAL_BUILD_SHIM_HWCSERVICE

        /// pointer to checker for the DRM shim
        DrmShimChecks* mDrmChecks;
        HwcTestKernel* mTestKernel;

    public:
        /// Constructor
        HwcTestState();
        /// Destructor
        ~HwcTestState();

        /// get singular instance
        static HwcTestState* getInstance();

        /// Initialize
        typedef void (*HwcShimInitFunc)();
        void CreateTestKernel();
        int LoggingInit(void* libHwcHandle);
        int TestStateInit(HwcShimInitializer* hwcShimInitializer);
        void RegisterWithHwc();
        void SetPreferences();

        /// Part of the closedown procedure for the harness
        void StopThreads();

        /// Static function called on image exit
        /// also can be called by harness to perform tidyup
        static void rundown(void);

    private:
        /// Static pointer to singular object
        static HwcTestState* mInstance;

        // For hooks into real HWC
        void *mLibHwcHandle;

        HwcShimInitializer* mHwcShimInitializer;

        // true if we are validating live state
        bool mLive;

        // Binder Interface functions
        android::sp<HwcShimService> mHwcShimService;

        /// Test configuration
        HwcTestConfig mConfig;

        /// Test results
        HwcTestResult mResult;

        /// Which shims are actually running (self-registered)
        uint32_t mRunningShims;

        /// Pointer to HWC add-to-log function
        HwcLogAddPtr mpHwcLogAdd;

        /// Pointer to HWC simulate hotplug function
        HwcSimulateHotPlugPtr mpHwcSimulateHotPlug;

        /// Number of hotplugs in progress (should only ever be 0 or 1)
        uint32_t mHotPlugInProgress;

        /// Default hotplug simulation state for new displays as HWC polls them for the first time
        bool mNewDisplayConnectionState;

        // used by the frame control class to signal to the binder when it is enabled
        bool mFrameControlEnabled;

        /// Lowest display index of an enabled display
        uint32_t mFirstDisplayWithVSync;

        // Handle for the Widi Visualisation layer. This is required so that it can
        // be ommited from the transform checks.
        buffer_handle_t mWidiVisualisationHandle;

        // Display spoof control
        HwcTestNullDisplaySpoof mDefaultDisplaySpoof;
        HwcTestDisplaySpoof* mDisplaySpoof;

        // DrmShimEventHandler restorer for vsync events
        HwcTestEventHandler* mVSyncRestorer;

        // Allow harness to synchronize with (real) OnSet
        Hwcval::Condition mOnSetCondition;
        Hwcval::Mutex mOnSetMutex;
        bool mOnSetConditionEnable;

        // Power suspend/resume mode
        bool mSuspend;

        // Define variables for the initial Widi Crtc settings
        uint32_t mWirelessCrtcWidth = 0;
        uint32_t mWirelessCrtcHeight = 0;
        uint32_t mWirelessCrtcRefresh = 0;

        // Define cached copies of the Widi Shim variables
        uint32_t mWirelessFrameCount = 0;
        uint32_t mWirelessFrameTypeChangeCount = 0;
        uint32_t mWirelessBufferInfoCount = 0;
        uint32_t mMaxSetResolutions = 10;
        bool mWidiDirectExpected = false;

        // Cache the start time for the last IVP_exec call. This is used by the
        // watchdog to detect lock-ups.
        Hwcval::Mutex mIVPStartTimeLock;
        uint64_t mIVPStartTime = 0LL;

        // MDS
        Hwcval::MultiDisplayInfoProviderShim* mMDSInfoProviderShim;

        // Target device type
        DeviceType mDeviceType;

        // Stalls
        Hwcval::Stall mStall[Hwcval::eStallMax];

        // Input frame to dump all buffers
        uint32_t mInputFrameToDump;

        // DRM function pointers
        FPdrm_intel_bo_map mfpdrm_intel_bo_map;
        FPdrm_intel_bo_unmap mfpdrm_intel_bo_unmap;

        // Dumping of input images
        android::sp<Hwcval::Selector> mFrameDumpSelector;
        uint32_t mMaxDumpImages;
        uint32_t mNumDumpImage;

        android::sp<Hwcval::Selector> mTgtFrameDumpSelector;

        // Scale factor range we will allow through to iVP
        float mMinIvpScale;
        float mMaxIvpScale;

        // Max latency allowed to unblank the display
        int64_t mMaxUnblankingLatency;

        // Transparent layer notified from harness
        buffer_handle_t mFutureTransparentLayer;

    public:
        // Accessors to get and set the device type (BYT/CHT/BXT)
        DeviceType GetDeviceType();
        void SetDeviceType(DeviceType device);

        /// Read/write access to test configuration
        void SetTestConfig(const HwcTestConfig& config);
        HwcTestConfig& GetTestConfig();

        // Are we validating live state (as opposed to a historic input file)
        bool IsLive();

        /// Get test result
        HwcTestResult& GetTestResult(void);

        /// Mark shim failure reason
        void SetShimFail(HwcTestCheckType feature);

        /// accessor for DRM checks object
        HwcTestKernel* GetTestKernel();

        /// access for HWC logger function
        HwcLogAddPtr GetHwcLogFunc();

        /// access for minimum Android log level
        int GetMinLogPriority();

        /// Reset results at start of the test
        void ResetTestResults();

        /// Send frame counts through to HwcTestResult
        void ReportFrameCounts(bool final=true);

        bool IsLoggingEnabled(int priority, HwcTestCheckType check);
        bool IsLoggingEnabled(int priority);
        bool IsCheckEnabled(HwcTestCheckType check);
        bool IsOptionEnabled(HwcTestCheckType check);
        int GetHwcOptionInt(const char* optionName);
        const char* GetHwcOptionStr(const char* optionName);
        bool IsBufferMonitorEnabled();
        bool IsAutoExtMode();
        unsigned GetSetDisplayCRCCheckDelay(unsigned batchDelay = unsigned(-1));
        bool ConfigRequiresFrameControl(const HwcTestConfig *pConfig = NULL) const;
        void SetFrameControlEnabled(bool);
        bool IsFrameControlEnabled() const;

        // Access HWC frame number
        uint32_t GetHwcFrame(uint32_t displayIx);

        // Power suspend/resume mode
        void SetSuspend(bool suspend);
        bool IsSuspended();

        void SetFirstDisplayWithVSync(uint32_t disp);
        uint32_t GetFirstDisplayWithVSync();
        int64_t GetVBlankTime(uint32_t displayIx, bool& enabled);

        // Wait for composition validation to complete
        // Allows test to ensure that all the important compositions get validated.
        void WaitForCompValToComplete();

        // Display property query
        uint32_t GetDisplayProperty(uint32_t displayIx, HwcTestState::DisplayPropertyType prop);

        // Override preferred HDMI mode
        void SetHdmiPreferredMode(uint32_t width=0, uint32_t height=0, uint32_t refresh=0);

        // Direct test kernel to simulate hot plug
        bool IsHotPluggableDisplayAvailable();
        bool SimulateHotPlug(bool connected, uint32_t displayTypes = 0xff);
        bool GetNewDisplayConnectionState();

        // Display is deemed to be in a bad state, we need to reboot
        bool IsTotalDisplayFail();

        // Expect PAVP teardown
        void NotifyProtectedContentChange();

        // Harness can call to avoid our work queue overflowing
        void ProcessWork();

        // Set up DRM/ADF failure spoofing
        void SetDisplaySpoof(HwcTestDisplaySpoof* displaySpoof);
        HwcTestDisplaySpoof& GetDisplaySpoof();

        // Vsync event restoration
        void SetVSyncRestorer(HwcTestEventHandler* restorer);
        void RestoreVSync(uint32_t disp);

        // Print out panel fitter statistics
        void ReportPanelFitterStatistics(FILE* f);

        //
        // Widi related methods
        //
        void SetWirelessCrtcParams(int32_t scaled_width, int32_t scaled_height, int32_t refresh);
        void SetWirelessFrameCount(uint32_t frame_count);
        uint32_t GetWirelessFrameCount();
        void SetWirelessFrameTypeChangeCount(uint32_t frame_type_change_count);
        uint32_t GetWirelessFrameTypeChangeCount();
        void SetWirelessBufferInfoCount(uint32_t buffer_info_count);
        uint32_t GetWirelessBufferInfoCount();
        void SetMaxSetResolutions(uint32_t max);
        uint32_t GetMaxSetResolutions();
        void SetWidiOutDimensions(uint32_t width, uint32_t height);
        void SetWidiDirectModeExpected(bool state);
        bool GetWidiDirectModeExpected();

        // Accessors for setting and getting the handle of the Widi Visualisation layer
        void SetWidiVisualisationHandle(buffer_handle_t handle);
        buffer_handle_t GetWidiVisualisationHandle(void);

        // Getters for the Wireless Crtc creation parameters
        uint32_t GetWirelessCrtcWidth() { return mWirelessCrtcWidth; }
        uint32_t GetWirelessCrtcHeight() { return mWirelessCrtcHeight; }
        uint32_t GetWirelessCrtcRefresh() { return mWirelessCrtcRefresh; }

        // iVP watchdog related calls
        void SetIVPCallTime(uint64_t time);
        uint64_t GetIVPStartTime();

        // Fence validity checking.
        // If we are not live, these always return true.
        bool IsFenceValid(int fence, uint32_t disp, uint32_t hwcFrame, bool checkSignalled=false, bool checkUnsignalled=false);
        bool IsFenceSignalled(int fence, uint32_t disp, uint32_t hwcFrame);
        bool IsFenceUnsignalled(int fence, uint32_t disp, uint32_t hwcFrame);

        // OnSet synchronization
        void TriggerOnSetCondition();
        void WaitOnSetCondition();

        // ESD Recovery
        void MarkEsdRecoveryStart(uint32_t connectorId);

        // MDS
        void SetMDSInfoProviderShim(Hwcval::MultiDisplayInfoProviderShim* shim);
        Hwcval::MultiDisplayInfoProviderShim* GetMDSInfoProviderShim();

        // Forced stalls for stability testing
        void SetStall(Hwcval::StallType ix, const Hwcval::Stall& stall);
        Hwcval::Stall& GetStall(Hwcval::StallType ix);

        // Configure image dump
        void ConfigureImageDump(android::sp<Hwcval::Selector> selector, uint32_t maxDumpImages);
        void ConfigureTgtImageDump(android::sp<Hwcval::Selector> selector);
        uint32_t TestImageDump(uint32_t hwcFrame);
        bool TestTgtImageDump(uint32_t hwcFrame);

        // Access to DRM function pointers
        void SetDrmFunctions(FPdrm_intel_bo_map fpdrm_intel_bo_map, FPdrm_intel_bo_unmap fpdrm_intel_bo_unmap);
        FPdrm_intel_bo_map GetDrmIntelBoMap();
        FPdrm_intel_bo_unmap GetDrmIntelBoUnmap();

        // So we can confirm if all the shims we expect are running
        void SetRunningShim(ShimMaskType shim);
        void CheckRunningShims(uint32_t mask);

        // Log to Kmsg (useful when DRM debug is enabled)
        void LogToKmsg(const char *format, ...);

        // Is a hot plug in progress?
        bool HotPlugInProgress();

        // Notification from harness that a layer in the next OnSet will be transparent
        void SetFutureTransparentLayer(buffer_handle_t handle);
        buffer_handle_t GetFutureTransparentLayer();

        // Tell the validation the rate we are (trying to) produce video at.
        // This can easily be broken by loading the system heavily or simply by running on a slow device
        // So will need to be careful on which tests these checks are enabled.
        void SetVideoRate(uint32_t disp, float videoRate);

        // Stringification
        static const char* DisplayTypeStr(uint32_t displayType);

        // Maximum scale factor we will allow through to iVP
        void SetIvpScaleRange(float minScale, float maxScale);
        float GetMinIvpScale();
        float GetMaxIvpScale();

        void SetMaxUnblankingLatency(int64_t ns);
        int64_t GetMaxUnblankingLatency();
};

inline void HwcTestState::SetDeviceType(DeviceType device)
{
    HWCLOGV("HwcTestState::SetDeviceType: setting device type to: %s",
            device == eDeviceTypeBYT ? "Baytrail" :
            device == eDeviceTypeCHT ? "Cherrytrail" :
            device == eDeviceTypeBXT ? "Broxton" : "Unknown");

    mDeviceType = device;
}

inline HwcTestState::DeviceType HwcTestState::GetDeviceType()
{
    return(mDeviceType);
}

inline bool HwcTestState::IsLive()
{
    return mLive;
}

inline void HwcTestState::SetTestConfig(const HwcTestConfig& config)
{
    HWCLOGD("HwcTestState::SetTestConfig");
    mConfig = config;
}

inline void HwcTestState::SetShimFail(HwcTestCheckType feature)
{
    mResult.SetFail(feature);
}

inline HwcTestKernel* HwcTestState::GetTestKernel()
{
    return mTestKernel;
}

inline HwcTestState::HwcLogAddPtr HwcTestState::GetHwcLogFunc()
{
    return mpHwcLogAdd;
}

inline int HwcTestState::GetMinLogPriority()
{
    return mConfig.mMinLogPriority;
}

inline bool HwcTestState::IsLoggingEnabled(int priority, HwcTestCheckType check)
{
    return ( mConfig.IsLevelEnabled(priority) && (mConfig.mCheckConfigs[check].enable) && mConfig.mGlobalEnable );
}

inline bool HwcTestState::IsLoggingEnabled(int priority)
{
    return ( mConfig.IsLevelEnabled(priority) );
}

inline bool HwcTestState::IsCheckEnabled(HwcTestCheckType check)
{
    return (  mConfig.mCheckConfigs[check].enable && mConfig.mGlobalEnable);
}

inline bool HwcTestState::IsOptionEnabled(HwcTestCheckType check)
{
    return (  mConfig.mCheckConfigs[check].enable);
}

inline bool HwcTestState::IsBufferMonitorEnabled()
{
    return mConfig.mBufferMonitorEnable;
}

inline void HwcTestState::SetSuspend(bool suspend)
{
    mSuspend = suspend;
}

inline bool HwcTestState::IsSuspended()
{
    return mSuspend;
}

inline HwcTestConfig* HwcGetTestConfig()
{
    return &(HwcTestState::getInstance()->GetTestConfig());
}

inline HwcTestResult* HwcGetTestResult()
{
    return &(HwcTestState::getInstance()->GetTestResult());
}

inline unsigned HwcTestState::GetSetDisplayCRCCheckDelay(unsigned batchDelay)
{
    if(batchDelay != unsigned(-1))
    {
        mConfig.mDisplayCRCCheckDelay = batchDelay;
    }
    return mConfig.mDisplayCRCCheckDelay;
}

inline bool HwcTestState::ConfigRequiresFrameControl(const HwcTestConfig *pConfig) const
{
    const HwcTestConfig *pCfg = pConfig ? pConfig : &mConfig;

    return pCfg->mGlobalEnable && pCfg->mCheckConfigs[eCheckCRC].enable;
}

inline void HwcTestState::SetFrameControlEnabled(bool enabled)
{
    mFrameControlEnabled = enabled;
}

inline bool HwcTestState::IsFrameControlEnabled() const
{
    return mFrameControlEnabled;
}

inline void HwcTestState::SetFirstDisplayWithVSync(uint32_t disp)
{
    HWCLOGD("First display with VSync=%d", disp);
    mFirstDisplayWithVSync = disp;
}

inline uint32_t HwcTestState::GetFirstDisplayWithVSync()
{
    return mFirstDisplayWithVSync;
}

inline void HwcTestState::SetDisplaySpoof(HwcTestDisplaySpoof* displaySpoof)
{
    if (displaySpoof)
    {
        mDisplaySpoof = displaySpoof;
    }
    else
    {
        mDisplaySpoof = &mDefaultDisplaySpoof;
    }
}

inline HwcTestDisplaySpoof& HwcTestState::GetDisplaySpoof()
{
    return *mDisplaySpoof;
}

inline void HwcTestState::SetVSyncRestorer(HwcTestEventHandler* restorer)
{
    // If allowed, setup the callback to restore VSync handling on VSync timeout
    if (mConfig.mCheckConfigs[eOptAutoRestoreVSync].enable)
    {
        mVSyncRestorer = restorer;
    }
}

inline void HwcTestState::RestoreVSync(uint32_t disp)
{
    if (mVSyncRestorer)
    {
        mVSyncRestorer->Restore(disp);
    }
}

// Accessors for setting and getting the handle of the Widi Visualisation layer
inline void HwcTestState::SetWidiVisualisationHandle(buffer_handle_t handle)
{
    mWidiVisualisationHandle = handle;
}

inline buffer_handle_t HwcTestState::GetWidiVisualisationHandle(void)
{
    return mWidiVisualisationHandle;
}

// Access to DRM function pointers
inline void HwcTestState::SetDrmFunctions(FPdrm_intel_bo_map fpdrm_intel_bo_map, FPdrm_intel_bo_unmap fpdrm_intel_bo_unmap)
{
    mfpdrm_intel_bo_map = fpdrm_intel_bo_map;
    mfpdrm_intel_bo_unmap = fpdrm_intel_bo_unmap;
}

inline HwcTestState::FPdrm_intel_bo_map HwcTestState::GetDrmIntelBoMap()
{
    return mfpdrm_intel_bo_map;
}

inline HwcTestState::FPdrm_intel_bo_unmap HwcTestState::GetDrmIntelBoUnmap()
{
    return mfpdrm_intel_bo_unmap;
}

inline void HwcTestState::SetRunningShim(ShimMaskType shim)
{
    mRunningShims |= (uint32_t) shim;
}

inline bool HwcTestState::HotPlugInProgress()
{
    return (mHotPlugInProgress != 0);
}

inline void HwcTestState::SetIvpScaleRange(float minScale, float maxScale)
{
    mMinIvpScale = minScale;
    mMaxIvpScale = maxScale;
}

inline float HwcTestState::GetMinIvpScale()
{
    return mMinIvpScale;
}

inline float HwcTestState::GetMaxIvpScale()
{
    return mMaxIvpScale;
}

inline void HwcTestState::SetMaxUnblankingLatency(int64_t ns)
{
    mMaxUnblankingLatency = ns;
}

inline int64_t HwcTestState::GetMaxUnblankingLatency()
{
    return mMaxUnblankingLatency;
}

inline bool HwcTestState::GetNewDisplayConnectionState()
{
    return mNewDisplayConnectionState;
}

#endif // __HwcTestState_h__
