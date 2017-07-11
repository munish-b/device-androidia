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

File Name:      HwcTestMdsControl.cpp

Description:    Shim of Multi-display service interface into HWC.

Environment:

****************************************************************************/

#include "HwcTestMdsControl.h"
#include "HwcTestState.h"
#include "HwcTestKernel.h"

#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE
#ifdef HWCVAL_MDSEXTMODECONTROL

HwcTestMdsControl::HwcTestMdsControl(sp<IMDSExtModeControl> real, HwcTestKernel* testKernel)
  : mReal(real),
    mTestKernel(testKernel)
{
    HWCLOGV("HwcTestMdsControl created @%p", this);
}

HwcTestMdsControl::~HwcTestMdsControl()
{
}

status_t HwcTestMdsControl::updateVideoState(int64_t videoSessionId, bool isPrepared)
{
    HWCLOGV_COND(eLogVideo, "IMDSExtModeControl::updateVideoState session %" PRIi64 " %s ENTER",
        videoSessionId, isPrepared ? "PREPARED" : "UNPREPARED");
    mTestKernel->UpdateVideoState(videoSessionId, isPrepared);

    status_t st = mReal->updateVideoState(videoSessionId, isPrepared);

    //HWCLOGV_COND(eLogVideo, "IMDSExtModeControl::updateVideoState session %" PRIi64 " %s EXIT",
    //    videoSessionId, isPrepared ? "PREPARED" : "UNPREPARED");

    return st;
}

status_t HwcTestMdsControl::updateVideoFPS(int64_t videoSessionId, int32_t fps)
{
    HWCLOGV_COND(eLogVideo, "IMDSExtModeControl::updateVideoFps session %" PRIi64 " fps %d ENTER",
        videoSessionId, fps);

    mTestKernel->UpdateVideoFPS(videoSessionId, fps);
    status_t st =  mReal->updateVideoFPS(videoSessionId, fps);

    //HWCLOGV_COND(eLogVideo, "IMDSExtModeControl::updateVideoFps session %" PRIi64 " fps %d EXIT",
    //    videoSessionId, fps);

    return st;
}

status_t HwcTestMdsControl::updateInputState(bool state)
{
    HWCLOGV_COND(eLogVideo, "IMDSExtModeControl: updateInputState %d ENTER", state);
    mTestKernel->UpdateInputState(state);
    status_t st = mReal->updateInputState(state);
    //HWCLOGV_COND(eLogVideo, "IMDSExtModeControl: updateInputState %d EXIT", state);
    return st;
}

#endif // HWCVAL_MDSEXTMODECONTROL
#endif // HWCVAL_BUILD_SHIM_HWCSERVICE
