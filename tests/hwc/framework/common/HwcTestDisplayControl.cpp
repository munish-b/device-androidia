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

#include "HwcTestDisplayControl.h"
#include "HwcTestState.h"
#include "HwcTestKernel.h"
#include "HwcTestCrtc.h"
#include "DrmShimChecks.h"

using namespace android;
using namespace ::intel::ufo::hwc::services;

#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE

HwcTestDisplayControl::HwcTestDisplayControl(uint32_t disp, sp<IDisplayControl> real, HwcTestKernel* testKernel)
  : mDisplayIx(disp),
    mReal(real),
    mTestKernel(testKernel)
{
    HWCLOGV("HwcTestDisplayControl created @%p", this);
}

HwcTestDisplayControl::~HwcTestDisplayControl()
{
}

status_t HwcTestDisplayControl::restoreAllDefaults()
{
    return mReal->restoreAllDefaults();
}


sp<IDisplayOverscanControl> HwcTestDisplayControl::getOverscanControl()
{
     return mReal->getOverscanControl();
}

sp<IDisplayScalingControl> HwcTestDisplayControl::getScalingControl()
{
    return mReal->getScalingControl();
}

sp<IDisplayModeControl> HwcTestDisplayControl::getModeControl()
{
    if (mDisplayModeControl == 0)
    {
        sp<IDisplayModeControl> realDispModeCtrl = mReal->getModeControl();
        mDisplayModeControl = new HwcTestDisplayModeControl(mDisplayIx, realDispModeCtrl, mTestKernel);
    }

    return mDisplayModeControl;
}

sp<IColorControl> HwcTestDisplayControl::getColorControl(int32_t param)
{
    return mReal->getColorControl(param);
}

sp<IDisplayBlankControl> HwcTestDisplayControl::getBlankControl()
{
    return mReal->getBlankControl();
}

HwcTestDisplayModeControl::HwcTestDisplayModeControl(uint32_t disp, sp<IDisplayModeControl> real, HwcTestKernel* testKernel)
  : mDisplayIx(disp),
    mReal(real),
    mTestKernel(testKernel)
{
    HWCLOGV("HwcTestDisplayModeControl created @%p", this);
    mCrtc = mTestKernel->GetHwcTestCrtcByDisplayIx(disp, true);
}

HwcTestDisplayModeControl::~HwcTestDisplayModeControl()
{
}

status_t HwcTestDisplayModeControl::restorePreferredMode()
{
    if (!mCrtc)
    {
        mCrtc = mTestKernel->GetHwcTestCrtcByDisplayIx(mDisplayIx, true);
    }

    if (mCrtc)
    {
        mCrtc->ClearUserMode();
    }
    else
    {
        HWCERROR(eCheckInternalError, "Can't restore preferred mode for display %d as no CRTC defined", mDisplayIx);
    }

    return mReal->restorePreferredMode();
}

Vector<IDisplayModeControl::Info> HwcTestDisplayModeControl::getAvailableModes()
{
    return mReal->getAvailableModes();
}

status_t HwcTestDisplayModeControl::getMode(uint32_t *width, uint32_t *height, uint32_t *refresh, uint32_t *flags, uint32_t *ratio)
{
    if (!width || !height || !refresh || !flags || !ratio)
    {
        return android::BAD_VALUE;
    }

    return mReal->getMode(width, height, refresh, flags, ratio);
}

status_t HwcTestDisplayModeControl::setMode(uint32_t width, uint32_t height, uint32_t refresh, uint32_t flags, uint32_t ratio)
{
    mCrtc = mTestKernel->GetHwcTestCrtcByDisplayIx(mDisplayIx, true);
    HWCLOGA("HwcTestDisplayModeControl::setMode Enter %dx%d refresh %d flags %d %s ratio %d",
        width, height, refresh, flags, DrmShimChecks::AspectStr(flags), ratio);

    if (mCrtc)
    {
        mCrtc->SetUserModeStart();
    }
    else
    {
        HWCLOGW("Can't set user mode for display %d as no CRTC defined", mDisplayIx);
    }

    status_t st = mReal->setMode(width, height, refresh, flags, ratio);
    mTestKernel->DoStall(Hwcval::eStallSetMode);

    if (mCrtc)
    {
        mCrtc->SetUserModeFinish(st, width, height, refresh, flags, ratio);
    }

    HWCLOGA("HwcTestDisplayModeControl::setMode Exit %dx%d refresh %d flags %d ratio %d",
        width, height, refresh, flags, ratio);
    return st;

};

#endif // HWCVAL_BUILD_SHIM_HWCSERVICE
