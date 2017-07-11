/****************************************************************************

Copyright (c) Intel Corporation (2015).

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

File Name:      HwcTestMdsControl.h

Description:    Shim of Multi-display service interface into HWC.

Environment:

****************************************************************************/

#ifndef __HwcTestMdsControl_h__
#define __HwcTestMdsControl_h__

#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE
#ifdef HWCVAL_MDSEXTMODECONTROL

#include "BxService.h"
#include "IDisplayControl.h"
#include "IDiagnostic.h"
#include "IMDSExtModeControl.h"

class HwcTestKernel;

using namespace intel::ufo::hwc::services;

class HwcTestMdsControl : public BnMDSExtModeControl
{
public:
    HwcTestMdsControl(sp<IMDSExtModeControl> real, HwcTestKernel* testKernel);
    virtual ~HwcTestMdsControl();
    virtual status_t updateVideoState(int64_t videoSessionId, bool isPrepared);
    virtual status_t updateVideoFPS(int64_t videoSessionId, int32_t fps);
    virtual status_t updateInputState(bool state);

private:
    sp<IMDSExtModeControl> mReal;
    HwcTestKernel* mTestKernel;
};

#endif // HWCVAL_MDSEXTMODECONTROL
#endif // HWCVAL_BUILD_SHIM_HWCSERVICE

#endif // __HwcTestMdsControl_h__
