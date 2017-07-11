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


#ifndef __IHwcShimService_h__
#define __IHwcShimService_h__

#include <binder/IInterface.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>

#include <utils/String8.h>

class HwcTestConfig;
class HwcTestResult;

namespace android {

    class IHwcShimService : public IInterface
    {
        public:
            /// binder transaction types
            enum
            {
                eALERT =IBinder::FIRST_CALL_TRANSACTION,
                eSET_HWC_TEST_CONFIG,
                eGET_HWC_TEST_CONFIG,
                eGET_HWC_TEST_RESULT
            };

            /// Get result of all checks
            virtual status_t GetHwcTestResult(HwcTestResult& result, bool disableAllChecks) = 0;
            /// Set test configuration including all check enables
            virtual status_t SetHwcTestConfig(const HwcTestConfig& config, bool resetResult) = 0;
            /// Get test configuration
            virtual status_t GetHwcTestConfig(HwcTestConfig& config) = 0;

            /// Android macro
            DECLARE_META_INTERFACE(HwcShimService);
    };

}


#endif // _HWC_SHIM_SERVICE_H__



