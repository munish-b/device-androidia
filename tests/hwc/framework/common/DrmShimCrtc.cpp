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

File Name:      DrmShimCrtc.cpp

Description:    Implmentation of DRMShimCrtc class.

Environment:

Notes:

****************************************************************************/
#include "DrmShimCrtc.h"
#include "DrmShimChecks.h"
#include "HwcTestUtil.h"
#include "HwcTestState.h"
#include "HwcvalContent.h"

#include <limits.h>
#include <drm_fourcc.h>

DrmShimCrtc::DrmShimCrtc(uint32_t crtcId, uint32_t width, uint32_t height, uint32_t clock, uint32_t vrefresh)
  : HwcTestCrtc(crtcId, width, height, clock, vrefresh),
    mChecks(0),
    mPipeIx(0),
    mVBlankFrame(0),
    mVBlankSignal(0),
    mPageFlipUserData(0),
    mBlankingFb(0)
{
    memset(&mVBlank, 0, sizeof(mVBlank));
}

DrmShimCrtc::~DrmShimCrtc()
{
}

// VBlank handling
drmVBlankPtr DrmShimCrtc::SetupVBlank()
{
    uint32_t flags = DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT;

    switch (mPipeIx)
    {
        case 1:
            flags |= DRM_VBLANK_SECONDARY;
            break;

        case 2:
            // Pipe index = 2
            flags |= ((2 << DRM_VBLANK_HIGH_CRTC_SHIFT) & DRM_VBLANK_HIGH_CRTC_MASK);
            break;

        default:
            break;
    }

    mVBlank.request.type = (drmVBlankSeqType) flags;
    mVBlank.request.sequence = 1;
    mVBlank.request.signal   = mCrtcId;

    HWCLOGV_COND(eLogEventHandler, "DrmShimCrtc::SetupVBlank mVBlank.request.type=0x%x .sequence=%d .signal=%p",
        mVBlank.request.type, mVBlank.request.sequence, mVBlank.request.signal);

    return &mVBlank;
}

drmVBlankPtr DrmShimCrtc::GetVBlank()
{
     return &mVBlank;
}

void DrmShimCrtc::SetUserVBlank(drmVBlankPtr vbl)
{
    if (vbl->request.type & DRM_VBLANK_RELATIVE)
    {
        mVBlankFrame = mFrame + vbl->request.sequence;
    }
    else
    {
        mVBlankFrame = vbl->request.sequence;
        // At this point could raise the event immediately if mVBlankFrame <= mFrame.
        // Would need to return a status and let the caller deal with it.
        // However no use is made of absolute counts as far as I know.
    }

    HWCLOGV_COND(eLogEventHandler, "DrmShimCrtc:: SetUserVBlank crtc %d VBlankFrame %d",
        mCrtcId, mVBlankFrame);

    mVBlankSignal = vbl->request.signal;
}

bool DrmShimCrtc::IsVBlankRequested(uint32_t frame)
{
    if ((frame >= mVBlankFrame) && (mVBlankSignal != 0))
    {
        mVBlankFrame = UINT_MAX;
        return true;
    }
    else
    {
        HWCLOGD_COND(eLogEventHandler, "IsVBlankRequested: No: crtc %d frame=%d, mVBlankFrame=%d, mVBlankSignal=0x%" PRIx64,
            mCrtcId, frame, mVBlankFrame, mVBlankSignal);
        return false;
    }
}

void* DrmShimCrtc::GetVBlankUserData()
{
    return (void*) mVBlankSignal;
}

// VBlank intercepted from DRM.
// Do any local actions we want, and decide if DRM shim should pass on the callback.
bool DrmShimCrtc::IssueVBlank(unsigned int frame, unsigned int sec, unsigned int usec, void *& userData)
{
    HWCLOGV_COND(eLogEventHandler, "DrmShimCrtc:: IssueVBlank crtc %d frame:%d VBlankFrame %d",mCrtcId, frame, mVBlankFrame);
    HWCVAL_UNUSED(sec);     // Possible future use
    HWCVAL_UNUSED(usec);    // Possible future use

    // No longer returning user data from here
    HWCVAL_UNUSED(userData);

    if (frame > mFrame)
    {
        // All we do now at this stage is make sure each VSync comes only once
        mFrame = frame;
        return true;
    }
    else
    {
        return false;
    }
}

void DrmShimCrtc::SavePageFlipUserData(uint64_t userData)
{
    HWCLOGV_COND(eLogEventHandler, "DrmShimCrtc::SavePageFlipUserData crtc %d userData %" PRIx64, mCrtcId, userData);
    mPageFlipUserData = userData;
}

uint64_t DrmShimCrtc::GetPageFlipUserData()
{
    ALOG_ASSERT(this);
    //HWCLOGV_COND(eLogEventHandler, "DrmShimCrtc::GetPageFlipUserData crtc %d userData %" PRIx64, mCrtcId, mPageFlipUserData);
    return mPageFlipUserData;
}

void DrmShimCrtc::DrmCallStart()
{
    mSetDisplayWatchdog.Start();
    mDrmCallStartTime = systemTime(SYSTEM_TIME_MONOTONIC);
}

int64_t DrmShimCrtc::GetDrmCallDuration()
{
    return systemTime(SYSTEM_TIME_MONOTONIC) - mDrmCallStartTime;
}

int64_t DrmShimCrtc::GetTimeSinceVBlank()
{
    return systemTime(SYSTEM_TIME_MONOTONIC) - mVBlankWatchdog.GetStartTime();
}

bool DrmShimCrtc::SimulateHotPlug(bool connected)
{
    HWCLOGD("Logically %sconnecting D%d crtc %d", connected ? "" : "dis", GetDisplayIx(), GetCrtcId());
    mSimulatedHotPlugConnectionState = connected;

    // Cancel any unblanking checks
    mUnblankingTime = 0;

    // We can't directly spoof the uevent that HWC receives so we tell HwcTestState
    // to make a direct call into HWC to simulate the hot plug.
    return false;
}

uint32_t DrmShimCrtc::SetDisplayEnter(bool suspended)
{
    if (HwcTestState::getInstance()->IsOptionEnabled(eOptPageFlipInterception))
    {
        HWCCHECK(eCheckDispGeneratesPageFlip);
        if ((mPageFlipTime < mPageFlipWatchdog.GetStartTime()) && (mSetDisplayCount > 1))
        {
            // No page flips since last setdisplay on this CRTC.
            // Probably means display is in a bad way.
            HWCERROR(eCheckDispGeneratesPageFlip, "Crtc %d: No page flip since %fs", mCrtcId, double(mPageFlipTime) / HWCVAL_SEC_TO_NS);
        }

        if (IsDisplayEnabled())
        {
            mPageFlipWatchdog.StartIfNotRunning();
        }
    }

    mSetDisplayCount++;

    mPowerStartSetDisplay = mPower;
    mSuspendStartSetDisplay = suspended;

    return mFramesSinceModeSet++;
}

void DrmShimCrtc::StopSetDisplayWatchdog()
{
    mSetDisplayWatchdog.Stop();
}

// When Drm SetDisplay is used, the main plane must always have a valid buffer on it even when it is turned off.
// This means that we must put a blanking buffer on the main plane in these circumstances.
// This is not a requirement for nuclear DRM. Hence in our translation layer we must allocate one ourselves.
//
// Here we allocate it and give it an FbId, just as HWC does when it uses SetDisplay.
uint32_t DrmShimCrtc::GetBlankingFb(
    /*::intel::ufo::gralloc::GrallocClient& gralloc,*/ DrmModeAddFB2Func
        addFb2Func,
    int fd) {
    if (mBlankingFb == 0)
    {
        // Allocate a blanking buffer
        // This is for nuclear spoof
        mBlankingBuffer = new android::GraphicBuffer(mWidth, mHeight, HAL_PIXEL_FORMAT_RGBA_8888,
                    GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_FB);

        ALOG_ASSERT(mBlankingBuffer.get());
        HWCLOGD("DrmShimCrtc::GetBlankingFb: Allocated blanking buffer %p", mBlankingBuffer->handle);
        if (mBlankingBuffer->handle == 0)
        {
            HWCERROR(eCheckDrmShimFail, "No blanking buffer allocated.");
            return 0;
        }

        uint32_t boHandle;
#if 0
        gralloc.getBufferObject(mBlankingBuffer->handle, &boHandle);
        HWCLOGD("DrmShimCrtc::GetBlankingFb: Blanking buffer has boHandle 0x%x", boHandle);
#endif
        uint32_t stride = mBlankingBuffer->stride * 4;
        uint32_t handles[4], pitches[4], offsets[4] = { 0 };
        handles[0] = boHandle;
        handles[1] = boHandle;
        handles[2] = boHandle;
        handles[3] = boHandle;
        pitches[0] = stride;
        pitches[1] = stride;
        pitches[2] = stride;
        pitches[3] = stride;
        int result = addFb2Func( fd, mWidth, mHeight, DRM_FORMAT_ABGR8888,
                             handles, pitches, offsets, &mBlankingFb, 0 );
        if (result != 0)
        {
            HWCERROR(eCheckDrmShimFail, "Failed to allocate FB ID to blanking buffer %dx%d boHandle 0x%x stride %d", mWidth, mHeight, boHandle, stride);
        }
        else
        {
            HWCLOGD("DrmShimCrtc::GetBlankingFb: Blanking buffer has FB %d", mBlankingFb);
        }
    }

    ALOG_ASSERT(mBlankingFb);

    return mBlankingFb;
}

const char* DrmShimCrtc::ReportSetDisplayPower(char* strbuf, uint32_t len)
{
    uint32_t n = snprintf(strbuf, len, "Enter(");
    if (n>= len) return strbuf;

    mPowerStartSetDisplay.Report(strbuf + n, len - n);
    n = strlen(strbuf);
    if (n>= len) return strbuf;

    n += snprintf(strbuf + n, len - n, ") Exit(");
    if (n>= len) return strbuf;

    GetPower().Report(strbuf + n, len - n);
    n = strlen(strbuf);
    if (n>= len) return strbuf;

    n += snprintf(strbuf + n, len - n, ")");

    return strbuf;
}

bool DrmShimCrtc::IsDRRSEnabled()
{
    ALOG_ASSERT(mChecks);
    return mChecks->IsDRRSEnabled(GetConnector());
}
