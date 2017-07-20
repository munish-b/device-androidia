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

#include "test_binder.h"
#include "HwcTestLog.h" // for logging

namespace android
{
IMPLEMENT_META_INTERFACE(HwcShimService, "HwcShimService");


BpHwcShimService::BpHwcShimService(const sp<IBinder>& impl)
: BpInterface<IHwcShimService>(impl)
{
}

BpHwcShimService::~BpHwcShimService()
{
    HWCLOGI("BpHwcShimService::~BpHwcShimService()");
}


status_t BpHwcShimService::SetHwcTestConfig(const HwcTestConfig& config, bool resetResult)
{
    android::Parcel dataToSend, reply;
    dataToSend.writeInterfaceToken(IHwcShimService::getInterfaceDescriptor());
    config.WriteToParcel(dataToSend);
    dataToSend.writeInt32(resetResult);
    status_t ret = remote()->transact(
                IHwcShimService::eSET_HWC_TEST_CONFIG, dataToSend, &reply);
    if (ret == android::NO_ERROR)
    {
        ret = reply.readInt32();
    }
    return ret;
}

status_t BpHwcShimService::GetHwcTestConfig(HwcTestConfig& config)
{
    android::Parcel dataToSend, reply;
    dataToSend.writeInterfaceToken(IHwcShimService::getInterfaceDescriptor());
    remote()->transact(IHwcShimService::eGET_HWC_TEST_CONFIG, dataToSend, &reply);
    config.ReadFromParcel(reply);
    uint32_t ret = reply.readInt32();
    return ret;
}

status_t BpHwcShimService::GetHwcTestResult(HwcTestResult& result, bool disableAllChecks)
{
    android::Parcel dataToSend, reply;
    dataToSend.writeInterfaceToken(IHwcShimService::getInterfaceDescriptor());
    dataToSend.writeInt32(disableAllChecks);
    remote()->transact(
            IHwcShimService::eGET_HWC_TEST_RESULT, dataToSend, &reply);
    result.ReadFromParcel(reply);
    uint32_t ret = reply.readInt32();
    return ret;
}

}
