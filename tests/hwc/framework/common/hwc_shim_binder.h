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

#ifndef __IHWC_SHIM_BINDER_H__
#define __IHWC_SHIM_BINDER_H__


#include "IHwcShimService.h"

class HwcTestState;
class HwcTestConfig;
class HwcTestResult;

namespace android {

    class BnHwcShimService : public BnInterface<android::IHwcShimService>
    {
       //TODO
       virtual status_t onTransact(uint32_t code, const Parcel& data,
                                    Parcel* reply, uint32_t flags);
    };

}

class HwcShimService : public android::BnHwcShimService
{
    public:
    HwcShimService(HwcTestState * pShim);
    virtual ~HwcShimService();

    /// pointer back to hwc shim
    HwcTestState * mpTestState;

    /// Get result of all checks
    virtual android::status_t GetHwcTestResult(HwcTestResult& result, bool disableAllChecks);
    /// Set test configuration including all check enables
    virtual android::status_t SetHwcTestConfig(const HwcTestConfig& config, bool resetResult);
    /// Get test configuration
    virtual android::status_t GetHwcTestConfig(HwcTestConfig& config);

    protected:
    void WaitForFrameControlFrameRelease();
};

#endif // _HWC_SHIM_SERVICE_H__



