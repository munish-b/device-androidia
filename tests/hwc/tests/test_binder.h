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

#ifndef __TEST_BINDER_H__
#define __TEST_BINDER_H__

#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include <utils/String8.h>
#include <utils/Log.h>

#include "IHwcShimService.h"

class HwcTestConfig;
class HwcTestResult;

using namespace android;

namespace android
{

    class BpHwcShimService : public BpInterface<IHwcShimService>
    {
        public:
            /// TODO comment
            BpHwcShimService(const sp<IBinder>& impl);

            ~BpHwcShimService();

            /// Get result of all checks
            virtual status_t GetHwcTestResult(HwcTestResult& result, bool disableAllChecks);
            /// Set test configuration including all check enables
            virtual status_t SetHwcTestConfig(const HwcTestConfig& config, bool resetResult);
            /// Get test configuration
            virtual status_t GetHwcTestConfig(HwcTestConfig& config);
    };

}
#endif // __TEST_BINDER_H__
