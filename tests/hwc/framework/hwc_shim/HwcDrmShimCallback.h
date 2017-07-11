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

Description:    Class definition for HWC Shim implementation
                of callbacks from DRM SHim

Environment:

Notes:

****************************************************************************/
#ifndef __HwcDrmShimCallback_h__
#define __HwcDrmShimCallback_h__

#include "DrmShimCallbackBase.h"

class HwcFrameControl;

typedef void (PFN_PFC)(uint32_t disp);

class HwcDrmShimCallback : public DrmShimCallbackBase
{
    public:
        HwcDrmShimCallback();
        virtual ~HwcDrmShimCallback();

        // VSync callback
        virtual void VSync(uint32_t disp);
        virtual void PageFlipComplete(uint32_t disp);

        void IncOnSetCounter();
        void SetPageFlipCompleteCallback(PFN_PFC *pfn);

    private:
        uint32_t    cHWCOnSets;
        uint32_t    cPageFlips;
        PFN_PFC     *pfnPageFlipCallback;

};

inline void HwcDrmShimCallback::IncOnSetCounter()
{
    ++cHWCOnSets;
}

inline void HwcDrmShimCallback::SetPageFlipCompleteCallback(PFN_PFC *pfn)
{
    pfnPageFlipCallback = pfn;
}

#endif // __HwcDrmShimCallback_h__
