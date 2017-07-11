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

File Name:      DrmShimCallbackBase.cpp

Description:    Class implementation for HWC Shim implementation
                of callbacks from DRM SHim

Environment:

Notes:

****************************************************************************/
#include <utils/Trace.h>

#include "HwcDrmShimCallback.h"
#include "HwcTestUtil.h"
#include "HwcTestDefs.h"

// Constructor
HwcDrmShimCallback::HwcDrmShimCallback()
    : cHWCOnSets(0),
      cPageFlips(0),
      pfnPageFlipCallback(NULL)
{
}

// Destructor
HwcDrmShimCallback::~HwcDrmShimCallback()
{
}

// Callbacks that can be overriden in subclass
void HwcDrmShimCallback::VSync(uint32_t disp)
{
    HWCVAL_UNUSED(disp);
}

void HwcDrmShimCallback::PageFlipComplete(uint32_t disp)
{
    HWCVAL_UNUSED(disp);
    ATRACE_CALL();

    ++cPageFlips;
    HWCLOGV("HwcDrmShimCallback::PageFlipComplete - OnSet/PageFlipComplete = %u/%u", cHWCOnSets, cPageFlips);
    if (pfnPageFlipCallback)
    {
        pfnPageFlipCallback(disp);
    }
}


