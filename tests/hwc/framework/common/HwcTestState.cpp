/****************************************************************************

Copyright (c) Intel Corporation (2014).

DISCLAIMER OF WARRANTY
NEITHER INTEL NOR ITS SUPPLIERS MAKE ANY REPRESENTATION OR WARRANTY OR
CONDITION OF ANY KIND WHETHER EXPRESS OR IMPLIED (EITHER IN FACT OR BY
OPERATION OF LAW) WITH RESPECT TO THE SOURCE CODE.  INTEL AND ITS SUPPLIERS
EXPRESSLY DISCLAIM ALL WARRANTIES OR CONDITIONS OF MERCHANTABILITY OR
FITNESS FOR A PARTICULAR PURPOSE.  INTEL AND ITS SUPPLIERS DO NOT WARRANT
THAT THE SOURCE CODE IS ERROR-FREE OR THAT OPERATION OF THE SOURCE CODE WILL
BE SECURE OR UNINTERRUPTED AND HEREBY DISCLAIM ANY AND ALL LIABILITY ON
ACCOUNT THEREOF.  THERE IS ALSO NO IMPLIED WARRANTY OF NON-INFRINGEMENT.
SOURCE CODE IS LICENSED TO LICENSEE ON AN "AS IS" BASIS AND NEITHER INTEL
NOR ITS SUPPLIERS WILL PROVIDE ANY SUPPORT, ASSISTANCE, INSTALLATION,
TRAINING OR OTHER SERVICES.  INTEL AND ITS SUPPLIERS WILL NOT PROVIDE ANY
UPDATES, ENHANCEMENTS OR EXTENSIONS.

File Name:      HwcTestState.cpp

Description:    Class implementation for HWCShim class.

Environment:

Notes:

****************************************************************************/


#undef LOG_TAG
#define LOG_TAG "HWC_SHIM"

#include <stdlib.h>
#include <dlfcn.h>
#include <cutils/properties.h>

#include "HwcTestDefs.h"

#include "hardware/hwcomposer_defs.h"
#include "GrallocClient.h"

#include "HwcTestState.h"
#include "hwc_shim_binder.h"

#include "DrmShimChecks.h"
#include "HwcTestConfig.h"
#include "HwcTestUtil.h"
#include "HwcShimInitializer.h"
#include "HwcvalServiceManager.h"
#include "HwcServiceShim.h"
#include "HwcvalThreadTable.h"

#ifdef HWCVAL_ABSTRACTLOG_EXISTS
#include "AbstractLog.h"
extern Hwcval::LogIntercept gLogIntercept;
#endif

#ifdef HWCVAL_ABSTRACTCOMPOSITIONCHECKER_EXISTS
// We support version 0 of AbstractCompositionChecker interface
#define ABSTRACTCOMPOSITIONCHECKER_VAL_VERSIONS_SUPPORTED (1<<0)
#endif

// HwcTestState Constructor
HwcTestState::HwcTestState()
  : mDrmChecks(0),
    mTestKernel(0),
    mHwcShimInitializer(0),
    mLive(true),
    mHwcShimService(0),
    mRunningShims(0),
    mpHwcLogAdd(0),
    mpHwcSimulateHotPlug(0),
    mHotPlugInProgress(0),
    mNewDisplayConnectionState(true),
    mFrameControlEnabled(false),
    mFirstDisplayWithVSync(0),
    mWidiVisualisationHandle(0),
    mDisplaySpoof(&mDefaultDisplaySpoof),
    mVSyncRestorer(0),
    mOnSetConditionEnable(false),
    mMDSInfoProviderShim(0),
    mDeviceType(eDeviceTypeUnknown),
    mfpdrm_intel_bo_map(0),
    mfpdrm_intel_bo_unmap(0),
    mMaxDumpImages(0),
    mNumDumpImage(0),
    mMinIvpScale(0),
    mMaxIvpScale(INFINITY),
    mMaxUnblankingLatency(HWCVAL_MAX_UNBLANKING_LATENCY_DEFAULT_US)
{
    Hwcval::InitThreadStates();
    SetShimFail(eCheckSFRestarted);
}

HwcTestState::~HwcTestState()
{
    HWCLOGI("Destroying HwcTestState...");
    delete mTestKernel;
    HWCLOGI("...Destroyed HwcTestState");
    mInstance = 0;
}

HwcTestState* HwcTestState::mInstance=0;

HwcTestState* HwcTestState::getInstance()
{
    if (mInstance==0)
    {
        mInstance = new HwcTestState();
    }
    return mInstance;
}

void HwcTestState::rundown()
{
    static volatile int sStateDeleted = 0;
    HWCLOGE("HwcTestState::rundown() - which means SF is exiting");

    if (android_atomic_swap(1, &sStateDeleted) == 0)
    {
        delete mInstance;
    }
}
#ifdef HWCVAL_ABSTRACTLOG_EXISTS
extern Hwcval::SetLogValPtr pfHwcLogSetLogVal;
#else
#ifdef HWCVAL_ABSTRACTCOMPOSITIONCHECKER_EXISTS
typedef uint32_t (*HwcLogSetCompositionCheckPtr) (intel::ufo::hwc::validation::AbstractCompositionChecker* compositionChecker);
static HwcLogSetCompositionCheckPtr pfHwcLogSetCompositionCheck = 0;
#endif
#endif


void HwcTestState::CreateTestKernel()
{
    mDrmChecks = new DrmShimChecks();
    mTestKernel = mDrmChecks;
    SetPreferences();
}

int HwcTestState::LoggingInit(void* libHwcHandle)
{
    mLibHwcHandle = libHwcHandle;
    const char* sym;

#ifndef HWCVAL_ABSTRACTLOG_EXISTS
    dlerror();
    sym = "hwcLogAdd";
    mpHwcLogAdd = (HwcLogAddPtr)dlsym(mLibHwcHandle, sym);

    if (mpHwcLogAdd)
    {
        (*mpHwcLogAdd)("HWC Shim connected to HWCLogAdd");
        HWCLOGI("HWC Shim connected to HWCLogAdd");
    }
    else
    {
        HWCLOGI("HWC Shim failed to connect to HWCLogAdd");
    }
#endif // HWCVAL_ABSTRACTLOG_EXISTS

#ifdef HWCVAL_ABSTRACTLOG_EXISTS
    sym = "hwcSetLogVal";
    pfHwcLogSetLogVal = (Hwcval::SetLogValPtr) dlsym(mLibHwcHandle, sym);

    // Enable HWCVAL logging to HWC
    CreateTestKernel();
    RegisterWithHwc();
#else
    CreateTestKernel();
#ifdef HWCVAL_ABSTRACTCOMPOSITIONCHECKER_EXISTS
    sym = "hwcLogSetCompositionCheck";
    pfHwcLogSetCompositionCheck = (HwcLogSetCompositionCheckPtr)dlsym(mLibHwcHandle, sym);
#endif
#endif

    return 0;
}

// Load HWC library and get hooks
// TODO move everything that can occur at construction time to the ctor
// use this for post construction settings from the test maybe rename.
int HwcTestState::TestStateInit(HwcShimInitializer* hwcShimInitializer)
{
    const char *sym;

    // TODO turn off some logging check android levels
    HWCLOGI("In HwcTestState Init");

    mHwcShimInitializer = hwcShimInitializer;

    dlerror();
    sym = "hwcSimulateHotPlug";
    mpHwcSimulateHotPlug = (HwcSimulateHotPlugPtr)dlsym(mLibHwcHandle, sym);

    if (mpHwcSimulateHotPlug)
    {
        HWCLOGI("HWC has hotplug simulation facility");
    }
    else
    {
        HWCLOGI("HWC does not have hotplug simulation facility");
    }

#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE
    // Start HWC service shim
    mHwcServiceShim = new HwcServiceShim();
    mHwcServiceShim->Start();
#endif // HWCVAL_BUILD_SHIM_HWCSERVICE

    // Create DRM Shim Checker Object, and give a pointer to the DRM Shim
    mTestKernel = mDrmChecks;

#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE
    mTestKernel->SetHwcServiceShim(mHwcServiceShim);
#endif // HWCVAL_BUILD_SHIM_HWCSERVICE

    // Set preferences from the environment
    SetPreferences();

    // Start service
#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE
    mHwcShimService = new HwcShimService(this);
#endif // HWCVAL_BUILD_SHIM_HWCSERVICE

    atexit(HwcTestState::rundown);

    return 0;
}

void HwcTestState::RegisterWithHwc()
{
#ifdef HWCVAL_ABSTRACTLOG_EXISTS
    // Tell Hwc we want to be informed of compositions
    if (pfHwcLogSetLogVal)
    {
        HWCLOGD("HwcTestState: Registering for log validation");
        intel::ufo::hwc::validation::AbstractCompositionChecker* compositionChecker = 0;
        Hwcval::LogChecker* logChecker = 0;

        if (mTestKernel)
        {
            compositionChecker = static_cast<intel::ufo::hwc::validation::AbstractCompositionChecker*> (mTestKernel);
            logChecker = mTestKernel->GetParser();
        }

        gLogIntercept.Register(logChecker, compositionChecker, ABSTRACTCOMPOSITIONCHECKER_VAL_VERSIONS_SUPPORTED);
    }
    else
    {
        HWCLOGD("HwcTestState: Can't register for composition check callbacks");
    }
#else
#ifdef HWCVAL_ABSTRACTCOMPOSITIONCHECKER_EXISTS
    // Tell Hwc we want to be informed of compositions
    if (pfHwcLogSetCompositionCheck)
    {
        HWCLOGD("HwcTestState: Registering for composition check callbacks");
        intel::ufo::hwc::validation::AbstractCompositionChecker* compositionChecker =
            static_cast<intel::ufo::hwc::validation::AbstractCompositionChecker*> (mTestKernel);
        uint32_t hwcSupportedVersionMask = (pfHwcLogSetCompositionCheck)(compositionChecker);

        if ((hwcSupportedVersionMask & ABSTRACTCOMPOSITIONCHECKER_VAL_VERSIONS_SUPPORTED) == 0)
        {
            (pfHwcLogSetCompositionCheck)(0);
            HWCERROR(eCheckInternalError, "AbstractCompositionChecker incompatible between HWC and validation.");
            HWCLOGE("  - Composition interception disabled, checks will fail.");
        }
    }
    else
    {
        HWCLOGD("HwcTestState: Can't register for composition check callbacks");
    }
#else
    HWCLOGD("Composition Check interface (AbstractCompositionChecker) unavailable");
#endif
#endif
}

void HwcTestState::SetPreferences()
{
    char modeStr[PROPERTY_VALUE_MAX];
    if (property_get( "hwcval.preferred_hdmi_mode", modeStr, NULL ))
    {
        // Mode format is <width>x<height>:<refresh rate>
        // You can use 0 for any value where you don't care.
        HWCLOGI("Processing hwcval.preferred_hdmi_mode=%s", modeStr);
        const char* p = modeStr;
        uint32_t w = atoiinc(p);
        if (*p++ != 'x')
        {
            return;
        }

        uint32_t h = atoiinc(p);
        if (*p++ != ':')
        {
            return;
        }
        uint32_t refresh = atoi(p);

        SetHdmiPreferredMode(w, h, refresh);
    }
}

HwcTestConfig& HwcTestState::GetTestConfig()
{
    return mConfig;
}

HwcTestResult& HwcTestState::GetTestResult()
{
    return mResult;
}

void HwcTestState::WaitForCompValToComplete()
{
    if (mTestKernel)
    {
        mTestKernel->WaitForCompValToComplete();
    }
}

// Display property query
uint32_t HwcTestState::GetDisplayProperty(uint32_t displayIx, DisplayPropertyType prop)
{
    if (mTestKernel)
    {
        return mTestKernel->GetDisplayProperty(displayIx, prop);
    }
    else
    {
        return 0;
    }
}

void HwcTestState::SetHdmiPreferredMode(uint32_t width, uint32_t height, uint32_t refresh)
{
    if (mTestKernel)
    {
        mTestKernel->SetHdmiPreferredMode(width, height, refresh);
    }
}

bool HwcTestState::IsHotPluggableDisplayAvailable()
{
    if (mTestKernel)
    {
        return mTestKernel->IsHotPluggableDisplayAvailable();
    }
    else
    {
        return false;
    }
}

// Direct test kernel to simulate hot plug
// returns true if hot plug was achieved
bool HwcTestState::SimulateHotPlug(bool connected, uint32_t displayTypes)
{
    if (mTestKernel)
    {
        if (connected && mTestKernel->IsHotPluggableDisplayAvailable())
        {
            // On hotplug, all existing protected sessions must be torn down.
            HWCLOGI("SimulateHotPlug: Encrypted sessions should be torn down soon.");
            mTestKernel->GetProtectionChecker().ExpectSelfTeardown();
        }

        bool hotPlugDone = mTestKernel->SimulateHotPlug(displayTypes, connected);

        if (!hotPlugDone)
        {
            // Direct call into HWC to pretend that a hotplug has been detected.
            // This to be used in DRM only, because under ADF we spoof the hotplug event.
            HWCLOGD_COND(eLogHotPlug, "Direct call into HWC to simulate hot%splug ENTER", connected ? "" : "un");
            ALOG_ASSERT(mpHwcSimulateHotPlug);
            ++mHotPlugInProgress;
            mpHwcSimulateHotPlug(connected);
            --mHotPlugInProgress;

            if (mTestKernel)
            {
                mTestKernel->DoStall(connected ? Hwcval::eStallHotPlug : Hwcval::eStallHotUnplug);
            }

            HWCLOGD_COND(eLogHotPlug, "Direct call into HWC to simulate hot%splug EXIT", connected ? "" : "un");
        }
    }
    else
    {
        HWCLOGW("No shims, can't simulate hot plug");
    }

    if (displayTypes & eRemovable)
    {
        mNewDisplayConnectionState = connected;
    }

    return true;
}

bool HwcTestState::IsTotalDisplayFail()
{
    if (mTestKernel)
    {
        return mTestKernel->IsTotalDisplayFail();
    }
    else
    {
        return false;
    }
}

// Expect a short period of inconsistency in protected content
void HwcTestState::NotifyProtectedContentChange()
{
    if (mTestKernel)
    {
        mTestKernel->GetProtectionChecker().TimestampChange();
    }
}

int64_t HwcTestState::GetVBlankTime(uint32_t displayIx, bool& enabled)
{
    if (mTestKernel)
    {
        HwcTestCrtc* crtc = mTestKernel->GetHwcTestCrtcByDisplayIx(displayIx);

        if (crtc)
        {
            int64_t t = crtc->GetVBlankTime(enabled);

            // If we haven't had a vblank, report when we started asking for them.
            return (t == 0) ? crtc->GetVBlankCaptureTime() : t;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

void HwcTestState::ProcessWork()
{
    if (mTestKernel)
    {
        mTestKernel->ProcessWork();
    }
}

void HwcTestState::ReportPanelFitterStatistics(FILE* f)
{
    if (mTestKernel)
    {
        for (uint32_t i=0; i<HWCVAL_MAX_CRTCS; ++i)
        {
            HwcTestCrtc* crtc = mTestKernel->GetHwcTestCrtcByDisplayIx(i);

            if (crtc)
            {
                crtc->ReportPanelFitterStatistics(f);
            }
        }
    }
}

void HwcTestState::ReportFrameCounts(bool final)
{
    if (mTestKernel)
    {
        if (final)
        {
            // Report if most recent ESD recovery event still not completed after timeout period.
            mTestKernel->EsdRecoveryReport();

            mTestKernel->FinaliseTest();
        }

        mTestKernel->SendFrameCounts(final);
    }
}

void HwcTestState::ResetTestResults()
{
    GetTestResult().Reset();
}

void HwcTestState::StopThreads()
{
    if (mTestKernel)
    {
        mTestKernel->StopThreads();
    }
}

bool HwcTestState::IsFenceValid(int fence, uint32_t disp, uint32_t hwcFrame, bool checkSignalled, bool checkUnsignalled)
{
    if (mLive)
    {
        if (fence < 0)
        {
            HWCLOGD_COND(eLogFence, "IsFenceValid: fence=-1, and we are looking for %s",
                checkSignalled ? "signalled so this is valid" : "not signalled so this is invalid");
            return checkSignalled;
        }
        else if (fence == 0)
        {
            return false;
        }

        struct sync_fence_info_data * fenceData = sync_fence_info(fence);

        if (fenceData == 0)
        {
            HWCERROR(eCheckFenceQueryFail, "Display %d frame:%d failed to query fence %d",
                disp, hwcFrame, fence);

            // We've already logged the error, so pretend everything is OK
            return true;
        }
        else if (fenceData->status < 0)
        {
            HWCERROR(eCheckInternalError, "Display %d frame:%d ERRONEOUS fence %d %s",
                disp, hwcFrame, fence, fenceData->name);
            sync_fence_info_free(fenceData);

            // We've already logged the error, so pretend everything is OK
            return true;
        }
        else if (fenceData->status == 0)
        {
            // not signalled
            sync_fence_info_free(fenceData);
            return !checkSignalled;
        }
        else
        {
            // signalled
            sync_fence_info_free(fenceData);
            return !checkUnsignalled;
        }
    }
    else
    {
        // We are not live, so we have to assume the fence is in the state we want it to be.
        return true;
    }
}

bool HwcTestState::IsFenceSignalled(int fence, uint32_t disp, uint32_t hwcFrame)
{
    return IsFenceValid(fence, disp, hwcFrame, true, false);
}

bool HwcTestState::IsFenceUnsignalled(int fence, uint32_t disp, uint32_t hwcFrame)
{
    return IsFenceValid(fence, disp, hwcFrame, false, true);
}

void HwcTestState::TriggerOnSetCondition()
{
    if (mOnSetConditionEnable)
    {
        HWCLOGD("HwcTestState::TriggerOnSetCondition");
        mOnSetCondition.signal();
    }
}

void HwcTestState::WaitOnSetCondition()
{
    mOnSetConditionEnable = true;
    Hwcval::Mutex::Autolock lock(mOnSetMutex);
    mOnSetCondition.waitRelative(mOnSetMutex, 1000 * HWCVAL_MS_TO_NS); // wait up to a second for OnSet
}

void HwcTestState::MarkEsdRecoveryStart(uint32_t connectorId)
{
    if (mTestKernel)
    {
        mTestKernel->MarkEsdRecoveryStart(connectorId);
    }
}

void HwcTestState::SetWirelessCrtcParams(int32_t scaled_width, int32_t scaled_height, int32_t refresh)
{
    mWirelessCrtcWidth = scaled_width;
    mWirelessCrtcHeight = scaled_height;
    mWirelessCrtcRefresh = refresh;
}

void HwcTestState::SetWirelessFrameCount(uint32_t frame_count)
{
    mWirelessFrameCount = frame_count;
}

uint32_t HwcTestState::GetWirelessFrameCount()
{
    return mWirelessFrameCount;
}

void HwcTestState::SetWirelessFrameTypeChangeCount(uint32_t
    frame_type_change_count)
{
    mWirelessFrameTypeChangeCount = frame_type_change_count;
}

uint32_t HwcTestState::GetWirelessFrameTypeChangeCount()
{
    return mWirelessFrameTypeChangeCount;
}

void HwcTestState::SetWirelessBufferInfoCount(uint32_t buffer_info_count)
{
    mWirelessBufferInfoCount = buffer_info_count;
}

uint32_t HwcTestState::GetWirelessBufferInfoCount()
{
    return mWirelessBufferInfoCount;
}

void HwcTestState::SetMaxSetResolutions(uint32_t max)
{
    mMaxSetResolutions = max;
}

uint32_t HwcTestState::GetMaxSetResolutions(void)
{
    return mMaxSetResolutions;
}

void HwcTestState::SetWidiDirectModeExpected(bool state)
{
    mWidiDirectExpected = state;
}

bool HwcTestState::GetWidiDirectModeExpected()
{
    return mWidiDirectExpected;
}

// Dynamically sets the dimensions for the Widi output buffer
void HwcTestState::SetWidiOutDimensions(uint32_t width, uint32_t height)
{
    if (mTestKernel)
    {
        HwcTestCrtc* crtc =
            mTestKernel->GetHwcTestCrtcByDisplayIx(HWCVAL_VD_WIDI_DISPLAY_INDEX);

        if (crtc)
        {
            HWCLOGD("Setting Widi out dimensions to %d %d", width, height);

            crtc->SetOutDimensions(width, height);
        }
        else
        {
            HWCLOGD("Could not find Widi CRTC (id: %d)", HWCVAL_VD_WIDI_CRTC_ID);
        }
    }
    else
    {
        HWCLOGD("Invalid pointer to test kernel");
    }
}

void HwcTestState::SetMDSInfoProviderShim(Hwcval::MultiDisplayInfoProviderShim* shim)
{
    mMDSInfoProviderShim = shim;
}

Hwcval::MultiDisplayInfoProviderShim* HwcTestState::GetMDSInfoProviderShim()
{
    return mMDSInfoProviderShim;
}

uint32_t HwcTestState::GetHwcFrame(uint32_t displayIx)
{
    if (mTestKernel)
    {
        return mTestKernel->GetHwcFrame(displayIx);
    }
    else
    {
        return 0;
    }
}

// iVP watchdog related calls
void HwcTestState::SetIVPCallTime(uint64_t time)
{
    mIVPStartTimeLock.lock();
    mIVPStartTime = time;
    mIVPStartTimeLock.unlock();
}

uint64_t HwcTestState::GetIVPStartTime()
{
    mIVPStartTimeLock.lock();
    uint64_t ret = mIVPStartTime;
    mIVPStartTimeLock.unlock();
    return ret;
}

void HwcTestState::SetStall(Hwcval::StallType ix, const Hwcval::Stall& stall)
{
    mStall[ix] = stall;
}

Hwcval::Stall& HwcTestState::GetStall(Hwcval::StallType ix)
{
    return mStall[ix];
}

// Configure image dump
void HwcTestState::ConfigureImageDump(android::sp<Hwcval::Selector> selector, uint32_t maxDumpImages)
{
    mFrameDumpSelector = selector;
    mMaxDumpImages = maxDumpImages;
}

void HwcTestState::ConfigureTgtImageDump(android::sp<Hwcval::Selector> selector)
{
    mTgtFrameDumpSelector = selector;
}

uint32_t HwcTestState::TestImageDump(uint32_t hwcFrame)
{
    if (mFrameDumpSelector.get() && mFrameDumpSelector->Test(hwcFrame))
    {
        if (++mNumDumpImage <= mMaxDumpImages)
        {
            return mNumDumpImage;
        }
    }

    // Don't dump.
    return 0;
}

bool HwcTestState::TestTgtImageDump(uint32_t hwcFrame)
{
    if (mTgtFrameDumpSelector.get())
    {
        return mTgtFrameDumpSelector->Test(hwcFrame);
    }

    return false;
}

void HwcTestState::CheckRunningShims(uint32_t mask)
{
    if ((mRunningShims & mask) != mask)
    {
        HWCERROR(eCheckSessionFail, "Shims running: 0x%x expected: 0x%x", mRunningShims, mask);
    }
}

void HwcTestState::LogToKmsg (const char *format, ...)
{
    FILE *kmsg = fopen("/dev/kmsg", "w");
    va_list vl;

    va_start(vl, format);
    if (kmsg)
    {
        vfprintf(kmsg, format, vl);
        fclose(kmsg);
    }
    va_end(vl);
}

int HwcTestState::GetHwcOptionInt(const char* str)
{
    if (mTestKernel)
    {
        return mTestKernel->GetHwcOptionInt(str);
    }
    else
    {
        return 0;
    }
}

const char* HwcTestState::GetHwcOptionStr(const char* str)
{
    if (mTestKernel)
    {
        return mTestKernel->GetHwcOptionStr(str);
    }
    else
    {
        return 0;
    }
}

bool HwcTestState::IsAutoExtMode()
{
    if (GetHwcOptionInt("extmodeauto"))
    {
        return true;
    }
    else
    {
        return IsOptionEnabled(eOptNoMds);
    }
}

// Notification from harness that a layer in the next OnSet will be transparent
void HwcTestState::SetFutureTransparentLayer(buffer_handle_t handle)
{
    mFutureTransparentLayer = handle;
}

buffer_handle_t HwcTestState::GetFutureTransparentLayer()
{
    return mFutureTransparentLayer;
}

void HwcTestState::SetVideoRate(uint32_t disp, float videoRate)
{
    if (mTestKernel)
    {
        mTestKernel->SetVideoRate(disp, videoRate);
    }
}

const char* HwcTestState::DisplayTypeStr(uint32_t displayType)
{
    switch(displayType)
    {
        case HwcTestState::eFixed:
            return "FIXED";

        case HwcTestState::eRemovable:
            return "REMOVABLE";

        case HwcTestState::eVirtual:
            return "VIRTUAL";

        default:
            return "MULTIPLE";
    }
}
