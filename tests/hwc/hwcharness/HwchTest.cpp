/****************************************************************************
*
* Copyright (c) Intel Corporation (2014).
*
* DISCLAIMER OF WARRANTY
* NEITHER INTEL NOR ITS SUPPLIERS MAKE ANY REPRESENTATION OR WARRANTY OR
* CONDITION OF ANY KIND WHETHER EXPRESS OR IMPLIED (EITHER IN FACT OR BY
* OPERATION OF LAW) WITH RESPECT TO THE SOURCE CODE.  INTEL AND ITS SUPPLIERS
* EXPRESSLY DISCLAIM ALL WARRANTIES OR CONDITIONS OF MERCHANTABILITY OR
* FITNESS FOR A PARTICULAR PURPOSE.  INTEL AND ITS SUPPLIERS DO NOT WARRANT
* THAT THE SOURCE CODE IS ERROR-FREE OR THAT OPERATION OF THE SOURCE CODE WILL
* BE SECURE OR UNINTERRUPTED AND HEREBY DISCLAIM ANY AND ALL LIABILITY ON
* ACCOUNT THEREOF.  THERE IS ALSO NO IMPLIED WARRANTY OF NON-INFRINGEMENT.
* SOURCE CODE IS LICENSED TO LICENSEE ON AN "AS IS" BASIS AND NEITHER INTEL
* NOR ITS SUPPLIERS WILL PROVIDE ANY SUPPORT, ASSISTANCE, INSTALLATION,
* TRAINING OR OTHER SERVICES.  INTEL AND ITS SUPPLIERS WILL NOT PROVIDE ANY
* UPDATES, ENHANCEMENTS OR EXTENSIONS.
*
* File Name:            HwchTest.cpp
*
* Description:          Abstract test class implementation
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#include "HwchTest.h"
#include "HwcTestLog.h"
#include "HwcTestState.h"
#include "HwcTestUtil.h"

#ifdef HWCVAL_TARGET_HAS_MULTIPLE_DISPLAY
#include "MultiDisplayShim.h"
#endif
#include "IService.h"
#ifdef HWCVAL_MDSEXTMODECONTROL
#include "IMDSExtModeControl.h"
#endif

#ifdef HWCVAL_BUILD_HWCSERVICE_API
#include "HwcServiceApi.h"
#endif

using namespace ::intel::ufo::hwc::services;


// TestParams class
// Encapsulates command-line options
Hwch::TestParams::TestParams()
  : mpParams(0)
{
}

Hwch::Test::Test(Hwch::Interface& interface)
  : mInterface(interface),
    mSystem(Hwch::System::getInstance())
{
    // No expectation as to cloning optimization since we can't
    // second guess how HWC will decide to perform cloning.
    // However, in a specific test, this can be set where cloning optimization is expected.
    SetExpectedMode(HwcTestConfig::eDontCare);
}

Hwch::Test::~Test()
{
#ifdef HWCVAL_BUILD_HWCSERVICE_API
    // Disconnect from the HWC Service Api
    if (mHwcsHandle)
    {
        HwcService_Disconnect(mHwcsHandle);
    }
#endif
}

void Hwch::TestParams::SetParams(ParamVec& params)
{
    mpParams = &params;
}

const char* Hwch::TestParams::GetParam(const char* name)
{
    if (!mpParams)
    {
        return 0;
    }

    ssize_t ix = mpParams->indexOfKey(android::String8(name));

    if (ix >= 0)
    {
        UserParam& param = mpParams->editValueAt(ix);
        param.mChecked = true;

        android::String8 paramStr;

        if (param.mValue == "1")
        {
            paramStr = android::String8::format("-%s ", name);
        }
        else
        {
            paramStr = android::String8::format("-%s=%s ", name, param.mValue.string());
        }

        if (mUsedArgs.find(paramStr) < 0)
        {
            mUsedArgs += paramStr;
        }

        return param.mValue.string();
    }
    else
    {
        return 0;
    }
}

const char* Hwch::TestParams::GetStrParam(const char* name, const char* deflt)
{
    // Safe for users as this will not return a null
    const char* str = GetParam(name);

    if (str)
    {
        return str;
    }
    else
    {
        return deflt;
    }
}

android::String8 Hwch::TestParams::GetStrParamLower(const char* name, const char* deflt)
{
    android::String8 result(GetStrParam(name, deflt));
    result.toLower();
    return result;
}



int Hwch::TestParams::GetIntParam(const char* name, int deflt)
{
    const char* str = GetParam(name);

    if (str)
    {
        return atoi(str);
    }
    else
    {
        return deflt;
    }
}

float Hwch::TestParams::GetFloatParam(const char* name, float deflt)
{
    const char* str = GetParam(name);

    if (str)
    {
        return atof(str);
    }
    else
    {
        return deflt;
    }
}

int64_t Hwch::TestParams::GetTimeParamUs(const char* name, int64_t deflt)
{
    const char* str = GetParam(name);
    const char* p = str;
    int64_t result = deflt;

    if (str)
    {
        double f = atofinc(p);

        if (strncmpinc(p, "s") == 0)
        {
            result = f * HWCVAL_SEC_TO_US;
        }
        else if (strncmpinc(p, "ms") == 0)
        {
            result = f * HWCVAL_MS_TO_US;
        }
        else if (strncmpinc(p, "us") == 0)
        {
            result = f;
        }
        else if (strncmpinc(p, "ns") == 0)
        {
            result = f / HWCVAL_US_TO_NS;
        }
        else
        {
            // Assume ms by default
            result = f * HWCVAL_MS_TO_US;
        }
    }

    return result;
}

// returns true if a valid range parameter (i.e. -param=x-y) is found, and the values in the last two arguments.
// default is INT_MIN to INT_MAX.
bool Hwch::TestParams::GetRangeParam(const char* name, Hwch::Range& range)
{
    const char* str = GetParam(name);

    if (str)
    {
        range = Hwch::Range(str);

        return true;
    }

    return false;
}

android::String8& Hwch::TestParams::UsedArgs()
{
    return mUsedArgs;
}

void Hwch::Test::SetName(const char* name)
{
    mName = name;
}

const char* Hwch::Test::GetName()
{
    return mName.string();
}

#ifdef HWCVAL_TARGET_HAS_MULTIPLE_DISPLAY
android::intel::IMultiDisplayCallback* Hwch::Test::MDSCallback()
{
    if (mMdsCbkShim.get() == 0)
    {
        android::sp<Hwcval::MultiDisplayCallbackRegistrarShim> reg = Hwcval::MultiDisplayCallbackRegistrarShim::getInstance();

        if (reg != 0)
        {
            mMdsCbkShim = reg->getCallback();
        }
    }
    return mMdsCbkShim.get();
}
#endif

bool Hwch::Test::CheckMDSAndSetup(bool report)
{
#ifdef HWCVAL_BUILD_HWCSERVICE_API
    if (mHwcsHandle)
    {
        return true; // Already connected
    }

    // Attempt to connect to the new HWC Service Api
    mHwcsHandle = HwcService_Connect();
    if (!mHwcsHandle)
    {
        HWCERROR(eCheckSessionFail, "HWC Service Api could not connect to service");
        return false;
    }

    return true;
#endif

#ifdef HWCVAL_MDSEXTMODECONTROL
    if (mMdsExtModeControl.get())
    {
        HWCLOGD_COND(eLogVideo, "Using mMdsExtModeControl=%p", mMdsExtModeControl.get());
        return true;
    }

    if (HwcTestState::getInstance()->IsOptionEnabled(eOptNewMds))
    {
        // Find and connect to HWC service
        sp<IBinder> hwcBinder = defaultServiceManager()->getService(String16(INTEL_HWC_SERVICE_NAME));
        sp<IService> hwcService = interface_cast<IService>(hwcBinder);
        if(hwcService == NULL)
        {
            HWCERROR(eCheckSessionFail, "Could not connect to service %s", INTEL_HWC_SERVICE_NAME);
            return false;
        }

        // Get MDSExtModeControl interface.
        mMdsExtModeControl = hwcService->getMDSExtModeControl();
        if (mMdsExtModeControl == NULL)
        {
            HWCERROR(eCheckSessionFail, "Cannot obtain IMDSExtModeControl");
            return false;
        }

        return true;
    }
    else
#endif
    {
#ifdef HWCVAL_TARGET_HAS_MULTIPLE_DISPLAY
        if (MDSCallback())
        {
            return true;
        }
        else
        {
            if (report)
            {
                HWCERROR(eCheckSessionFail, "MDS Shim Callback is required for this test");
            }
            return false;
        }
#else
        if (report)
        {
            HWCERROR(eCheckSessionFail, "Test requires MDS which is not supported on this platform");
        }

        return false;
#endif
    }
}

bool Hwch::Test::IsAutoExtMode()
{
    return (HwcTestState::getInstance()->IsAutoExtMode());
}

status_t Hwch::Test::UpdateVideoState(int sessionId, bool isPrepared, uint32_t fps)
{
    if (IsAutoExtMode())
    {
        // In No-MDS mode there are no video sessions
        return OK;
    }

    status_t st = NAME_NOT_FOUND;

    if (CheckMDSAndSetup(false))
    {
#ifdef HWCVAL_BUILD_HWCSERVICE_API
        st = HwcService_MDS_UpdateVideoState(mHwcsHandle, sessionId, isPrepared ? HWCS_TRUE : HWCS_FALSE);

        if (st == 0)
        {
            st = HwcService_MDS_UpdateVideoFPS(mHwcsHandle, sessionId, fps);
        }
#endif

#ifdef HWCVAL_MDSEXTMODECONTROL
        if (mMdsExtModeControl.get())
        {
            st = mMdsExtModeControl->updateVideoState(sessionId, isPrepared);

            if (st == 0)
            {
                st = mMdsExtModeControl->updateVideoFPS(sessionId, fps);
            }
        }
        else
#endif
        {
#ifdef HWCVAL_TARGET_HAS_MULTIPLE_DISPLAY
            st = MDSCallback()->updateVideoState(sessionId,
                                        (isPrepared ? android::intel::MDS_VIDEO_PREPARED :
                                                      android::intel::MDS_VIDEO_UNPREPARED));

            if (st == 0)
            {
                Hwcval::MultiDisplayInfoProviderShim* shim = HwcTestState::getInstance()->GetMDSInfoProviderShim();

                if (shim)
                {
                    // Note: only the frameRate will be used, as currently understood.
                    MDSVideoSourceInfo videoSource;
                    videoSource.displayW=1920;
                    videoSource.displayH=1080;
                    videoSource.frameRate = fps;
                    videoSource.isInterlaced = false;
                    videoSource.isProtected = false;

                    // If we are saying a video is being played, tell the shim the frame rate for this video source
                    // otherwise tell it there is no valid session playing (-1 won't match a session).
                    shim->SetShimVideoSourceInfo(isPrepared ? sessionId : -1,
                                                 &videoSource);
                }
                else
                {
                    HWCERROR(eCheckSessionFail, "No MDS Info provider shim");
                }
            }
#endif
        }
    }

    return st;
}

status_t Hwch::Test::UpdateInputState(bool inputActive, bool expectPanelEnableAsInput, Hwch::Frame* frame)
{
    if (IsAutoExtMode())
    {
        HWCLOGD("UpdateInputState: extmodeauto: inputActive %d expectPanelEnableAsInput %d",
            inputActive, expectPanelEnableAsInput);

        if (expectPanelEnableAsInput)
        {
            SetExpectedMode(HwcTestConfig::eDontCare);
        }

        // Turn the keypress generator on or off as appropriate
        mSystem.GetInputGenerator().SetActive(inputActive);

        if (frame)
        {
            frame->Send(10);
        }

        mSystem.GetInputGenerator().Stabilize();

        if (expectPanelEnableAsInput)
        {
            SetExpectedMode(inputActive ? HwcTestConfig::eOn : HwcTestConfig::eOff);
        }

        return OK;
    }
    else
    {
        HWCLOGD("UpdateInputState: NOT extmodeauto: inputActive %d expectPanelEnableAsInput %d",
            inputActive, expectPanelEnableAsInput);

    }

    if (CheckMDSAndSetup(false))
    {
        if (expectPanelEnableAsInput)
        {
            SetExpectedMode(inputActive ? HwcTestConfig::eOn : HwcTestConfig::eOff);
        }

#ifdef HWCVAL_BUILD_HWCSERVICE_API
#ifndef HWCVAL_TARGET_HAS_MULTIPLE_DISPLAY
        return HwcService_MDS_UpdateInputState(mHwcsHandle, inputActive ? HWCS_TRUE : HWCS_FALSE);
#endif
#endif

#ifdef HWCVAL_MDSEXTMODECONTROL
        if (mMdsExtModeControl.get())
        {
             return mMdsExtModeControl->updateInputState(inputActive);
        }
        else
#endif
        {
#ifdef HWCVAL_TARGET_HAS_MULTIPLE_DISPLAY
            return MDSCallback()->updateInputState(inputActive);
#endif
        }
    }

    return NAME_NOT_FOUND;
}

void Hwch::Test::SetExpectedMode(HwcTestConfig::PanelModeType modeExpect)
{
    HWCLOGV_COND(eLogVideo, "Hwch::Test::SetExpectedMode %s", HwcTestConfig::Str(modeExpect));
    HwcGetTestConfig()->SetModeExpect(modeExpect);
}

HwcTestConfig::PanelModeType Hwch::Test::GetExpectedMode()
{
    return HwcGetTestConfig()->GetModeExpect();
}

bool Hwch::Test::SimulateHotPlug(bool connected, uint32_t displayTypes, uint32_t delayUs)
{
    AsyncEvent::Data* pData = new AsyncEvent::HotPlugEventData(displayTypes);
    return SendEvent(connected ? AsyncEvent::eHotPlug : AsyncEvent::eHotUnplug, pData, delayUs);
}

bool Hwch::Test::GetVideoControl()
{
    if (mVideoControl.get() == 0)
    {
        // Find and connect to HWC service
        sp<android::IBinder> hwcBinder = defaultServiceManager()->getService(String16(INTEL_HWC_SERVICE_NAME));
        sp<IService> hwcService = interface_cast<IService>(hwcBinder);
        if(hwcService == NULL)
        {
            HWCERROR(eCheckSessionFail, "Could not connect to service %s", INTEL_HWC_SERVICE_NAME);
            return false;
        }

        // Get MDSExtModeControl interface.
        mVideoControl = hwcService->getVideoControl();
        if (mVideoControl == NULL)
        {
            HWCERROR(eCheckSessionFail, "Cannot obtain IVideoControl");
            return false;
        }
    }

    return true;
}

bool Hwch::Test::SetVideoOptimizationMode(Display::VideoOptimizationMode videoOptimizationMode, uint32_t delayUs)
{
#ifndef HWCVAL_BUILD_HWCSERVICE_API
    if (!GetVideoControl())
    {
        return false;
    }
#endif

    android::sp<Hwch::AsyncEvent::VideoOptimizationModeData> params =
        new Hwch::AsyncEvent::VideoOptimizationModeData(mVideoControl, videoOptimizationMode);

    return Hwch::System::getInstance().AddEvent(Hwch::AsyncEvent::eSetVideoOptimizationMode, params, delayUs);
}

void Hwch::Test::SetCheckPriority(HwcTestCheckType check, int priority)
{
    HwcTestResult& result = *HwcGetTestResult();

    if (check < eHwcTestNumChecks)
    {
        result.mFinalPriority[check] = priority;
    }
}

void Hwch::Test::SetCheck(HwcTestCheckType check, bool enable)
{
    HwcTestResult& result = *HwcGetTestResult();
    HwcTestConfig& config = *HwcGetTestConfig();

    if (check < eHwcTestNumChecks)
    {
        config.mCheckConfigs[check].enable = enable;
        result.mCausesTestFail[check] &= enable;
    }
}

// Set check priority conditionally to reducedPriority if failure count <= maxNormCount
void Hwch::Test::ConditionalDropPriority(HwcTestCheckType check, uint32_t maxNormCount, int reducedPriority)
{
    HwcGetTestResult()->ConditionalDropPriority(check, maxNormCount, reducedPriority);
}

bool Hwch::Test::IsAbleToRun()
{
    return true;
}

bool Hwch::Test::IsOptionEnabled(HwcTestCheckType check)
{
    return HwcTestState::getInstance()->IsOptionEnabled(check);
}

// Generate an event.
// delayUs is negative to happen immediately on the main thread;
// zero to happen immediately on the event generator thread;
// positive to happen after the stated delay on the event generator thread.
//
bool Hwch::Test::SendEvent(uint32_t eventType, int32_t delayUs)
{
    return mSystem.AddEvent(eventType, delayUs);
}

bool Hwch::Test::SendEvent(uint32_t eventType,
    android::sp<Hwch::AsyncEvent::Data> eventData, int32_t delayUs)
{
    return mSystem.AddEvent(eventType, eventData, delayUs);
}

bool Hwch::Test::Blank(bool blank,          // true for blank, false for unblank
                       bool power,          // whether to update the power state to match
                       int32_t delayUs)
{
    uint32_t t = blank ? AsyncEvent::eBlank : AsyncEvent::eUnblank;

    if (power)
    {
        t |= blank ? AsyncEvent::eSuspend : AsyncEvent::eResume;
    }

    return SendEvent(t, delayUs);
}

// Add an API function to enter/exit wireless docking mode.
// Note: this will only work if a Widi connection is present.
bool Hwch::Test::WirelessDocking(bool entry, int32_t delayUs)
{
    return SendEvent(entry ? AsyncEvent::eWirelessDockingEntry :
        AsyncEvent::eWirelessDockingExit, delayUs);
}

int Hwch::Test::Run()
{
#ifdef TARGET_HAS_MCG_WIDI
    // Open the Widi session (if required)
    Hwch::Widi& widi = Hwch::Widi::getInstance();
    if (mSystem.IsWirelessDisplayEmulationEnabled())
    {
        widi.WidiConnect();
    }
#endif

    // Unblank if previously blanked
    for (uint32_t i=0; i<HWCVAL_MAX_CRTCS; ++i)
    {
        if (mSystem.GetDisplay(i).IsConnected())
        {
            if (mInterface.IsBlanked(i))
            {
                mInterface.Blank(i, 0);
            }
        }
    }

    if (IsAutoExtMode())
    {
        // Stop us dropping into extended mode if we don't want to
        mSystem.GetInputGenerator().SetActive(true);
    }

    // retrieve Reference Composer composition flag
    mSystem.SetNoCompose(GetParam("no_compose") != 0);

    RunScenario();

    // Send a blank frame to allow buffers used in the test to be deleted
    Frame(mInterface).Send();
    mSystem.FlushRetainedBufferSets();

    if (GetParam("blank_after"))
    {
        for (uint32_t i=0; i<HWCVAL_MAX_CRTCS; ++i)
        {
            if (mSystem.GetDisplay(i).IsConnected())
            {
                mInterface.Blank(i, 1);
            }
        }
    }

#ifdef TARGET_HAS_MCG_WIDI
    // Close the Widi session (if required)
    if (mSystem.IsWirelessDisplayEmulationEnabled())
    {
        widi.WidiDisconnect();
    }
#endif

    return 0;
}

Hwch::BaseReg* Hwch::BaseReg::mHead = 0;

Hwch::BaseReg::~BaseReg()
{
}

Hwch::OptionalTest::OptionalTest(Hwch::Interface& interface)
  : Hwch::Test(interface)
{
}

bool Hwch::OptionalTest::IsAbleToRun()
{
    return false;
}

