/****************************************************************************
*
* Copyright (c) Intel Corporation (DATE_TO_BE_CHNAGED).
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
* File Name:            test_binder.h
*
* Description:
*
* Environment (opt):
*
* Notes (opt):
*
*****************************************************************************/

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
