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

File Name:      DrmShimCallbackBase.h

Description:    Class definition for base class allowing HWC shim to
                register for callbacks from DRM Shim

Environment:

Notes:

****************************************************************************/
#ifndef __DrmShimCallbackBase_h__
#define __DrmShimCallbackBase_h__

#include "HwcTestState.h"

#define HWCVAL_DRMSHIMCALLBACKBASE_VERSION 2

class DrmShimCallbackBase
{
    public:
        virtual ~DrmShimCallbackBase();

        bool CheckVersion();
        uint32_t GetVersion();

        // Callbacks that can be overriden in subclass
        virtual void VSync(uint32_t disp);
        virtual void PageFlipComplete(uint32_t disp);
};

inline bool DrmShimCallbackBase::CheckVersion()
{
    bool result = (GetVersion() == HWCVAL_DRMSHIMCALLBACKBASE_VERSION);

    if (!result)
    {
        HWCERROR(eCheckDrmShimFail, "Incompatible shims");
    }

    return result;
}



#endif // __DrmShimCallbackBase_h__
