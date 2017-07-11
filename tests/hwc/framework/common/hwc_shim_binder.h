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

File Name:      hwc_shim_binder.h

Description:    Hwc Shim binder interface.

Environment:

Notes:

****************************************************************************/


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



