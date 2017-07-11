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

Description:    Class implementation for base class allowing HWC shim to
                register for callbacks from DRM Shim

Environment:

Notes:

****************************************************************************/
#include "DrmShimCallbackBase.h"
#include "HwcTestUtil.h"

DrmShimCallbackBase::~DrmShimCallbackBase()
{
}

uint32_t DrmShimCallbackBase::GetVersion()
{
    return HWCVAL_DRMSHIMCALLBACKBASE_VERSION;
}

// Callbacks that can be overriden in subclass
void DrmShimCallbackBase::VSync(uint32_t disp)
{
    HWCVAL_UNUSED(disp);
}

void DrmShimCallbackBase::PageFlipComplete(uint32_t disp)
{
    HWCVAL_UNUSED(disp);
}


