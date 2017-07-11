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

File Name:      hwc_shim_binder.cpp

Description:    Hwc binder interface.
                See hwc_shim_binder.h for descriptions of functions.

Environment:

Notes:

****************************************************************************/

#include "hwc_shim_binder.h"
#include "HwcTestState.h"
#include "HwcTestUtil.h"
#include "DrmShimChecks.h"

#undef LOG_TAG
#define LOG_TAG "HWC_SHIM"

namespace android
{
    /// TODO comment
    class BpHwcShimService : public BpInterface<IHwcShimService>
    {
        public:
            BpHwcShimService(const sp<IBinder>& impl)
                : BpInterface<IHwcShimService>(impl)
                {
                }

            /// Get result of all checks
            virtual status_t GetHwcTestResult(HwcTestResult& result, bool disableAllChecks)
            {
                HWCVAL_UNUSED(result);
                HWCVAL_UNUSED(disableAllChecks);
                return 0;
            }

            /// Set test configuration including all check enables
            virtual status_t SetHwcTestConfig(const HwcTestConfig& config, bool resetResult)
            {
                HWCVAL_UNUSED(config);
                HWCVAL_UNUSED(resetResult);
                return 0;
            }

            /// Get test configuration
            virtual status_t GetHwcTestConfig(HwcTestConfig& config)
            {
                HWCVAL_UNUSED(config);
                return 0;
            }
    };

    IMPLEMENT_META_INTERFACE(HwcShimService, "HwcShimService");

    status_t BnHwcShimService::onTransact(uint32_t code, const Parcel& data,
            Parcel* reply, uint32_t flags)
    {
        HWCLOGI("BnHwcShimService::onTransact code=%d",code);
        data.checkInterface(this);
        // return ret for error
        status_t ret;

        switch (code)
        {
            case eGET_HWC_TEST_CONFIG:
            {
                HwcTestConfig config;
                ret = GetHwcTestConfig(config);
                config.WriteToParcel(*reply);
                reply->writeInt32(ret);
                return android::NO_ERROR;
            }

            case eSET_HWC_TEST_CONFIG:
            {
                HwcTestConfig config;
                bool resetResult;
                config.ReadFromParcel(data);
                resetResult = data.readInt32();

                ret = SetHwcTestConfig(config, resetResult);
                reply->writeInt32(ret);
                return android::NO_ERROR;
            }

            case eGET_HWC_TEST_RESULT:
            {
                HwcTestResult result;
                bool disableAllChecks = data.readInt32();

                ret = GetHwcTestResult(result, disableAllChecks);
                result.WriteToParcel(*reply);
                reply->writeInt32(ret);
                return android::NO_ERROR;
            }

            default:
                HWCLOGI("HwcService onTransact default");
                return android::BBinder::onTransact(code, data, reply, flags);
        }
    }
}

HwcShimService::HwcShimService(HwcTestState * pShim):
   mpTestState(pShim)
{
    HWCLOGI("Adding HWC Service");
    android::sp<android::IServiceManager> sm = realServiceManager();
    if(sm->addService(android::String16("HwcShimService"), this, false))
    {
        HWCERROR(eCheckInternalError, "Could not start hwc binder service");
    }
}

HwcShimService::~HwcShimService()
{
    HWCLOGI("HwcShimService::~HwcShimService()");
}

android::status_t HwcShimService::GetHwcTestConfig(HwcTestConfig& config)
{
    config = mpTestState->GetTestConfig();
    return android::NO_ERROR;
}

void HwcShimService::WaitForFrameControlFrameRelease()
{
    const int cLoopWaitMilliseconds = 100;
    const int cLoopMax = 10;
    int loop;

    HWCLOGD("HwcShimService::WaitForFrameControlFrameRelease - called");
    mpTestState->GetTestConfig().SetCheck(eCheckCRC, false);
    for(loop = 0; loop < cLoopMax && mpTestState->IsFrameControlEnabled(); ++loop)
    {
        usleep(cLoopWaitMilliseconds * 1000);
    }

    if(loop == cLoopMax)
    {
        HWCLOGW("HwcShimService::WaitForFrameControlFrameRelease - ERROR: TIMED OUT (after %dms) WAITING FOR FRAME RELEASE", loop * cLoopWaitMilliseconds);
    }

    HWCLOGD("HwcShimService::WaitForFrameControlFrameRelease - released after %dms", loop * cLoopWaitMilliseconds);
}

android::status_t HwcShimService::SetHwcTestConfig(const HwcTestConfig& config, bool resetResult)
{
    bool waitForFrameRelease = false;

    if(mpTestState->ConfigRequiresFrameControl() &&
       !mpTestState->ConfigRequiresFrameControl(&config))
    {
        // the new config no longer requires frame control. Once the test
        // config has been updated we will wait for the frame to be released
        waitForFrameRelease = true;
    }

    if(waitForFrameRelease)
    {
        WaitForFrameControlFrameRelease();
    }

    // update the state
    mpTestState->SetTestConfig(config);

    if (resetResult)
    {
        mpTestState->ResetTestResults();
    }

    mpTestState->GetTestKernel()->SendFrameCounts(resetResult);

    return android::NO_ERROR;
}

android::status_t HwcShimService::GetHwcTestResult(HwcTestResult& result, bool disableAllChecks)
{
    HWCLOGD("HwcShimService::GetHwcTestResult - called");
    mpTestState->GetTestKernel()->SendFrameCounts(false);

    if (disableAllChecks)
    {
        bool waitForFrameRelease = false;
        if(mpTestState->ConfigRequiresFrameControl())
        {
            // the new config no longer requires frame control. Once the test
            // config has been updated we will wait for the frame to be released
            waitForFrameRelease = true;
        }
        if(waitForFrameRelease)
        {
            HWCLOGD("HwcShimService::GetHwcTestResult - waiting for frame control release");
            WaitForFrameControlFrameRelease();
        }

        HWCLOGD("HwcShimService::GetHwcTestResult - disabling all checks");
        mpTestState->GetTestConfig().DisableAllChecks();
    }

    HWCLOGD("HwcShimService::GetHwcTestResult - getting test results");
    result = mpTestState->GetTestResult();
    result.SetEndTime();
    HWCLOGD("HwcShimService::GetHwcTestResult - returning");
    return android::NO_ERROR;
}
