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

File Name:      DrmShimPlane.cpp

Description:    Implmentation of DRMShimPlane class.

Environment:

Notes:

****************************************************************************/
#include "DrmShimPlane.h"
#include "HwcTestCrtc.h"
#include "DrmShimBuffer.h"
#include "HwcTestState.h"
#include "HwcTestUtil.h"

#undef LOG_TAG
#define LOG_TAG "DRM_SHIM"
#include <cutils/log.h>
#include <utils/Timers.h>
#include <drm_fourcc.h>

DrmShimPlane::DrmShimPlane(uint32_t planeId)
  : mPlaneId(planeId),
    mPlaneIx(0),
    mDsId(0),
    mCrtc(0),
    mRedrawExpected(false),
    mSetDisplayFailed(false),
    mBpp(0),    // No Bpp set on plane initially so 1st set plane will generate potential flicker
    mPixelFormat(0),
    mDrmCallStartTime(0),
    mBufferUpdated(false)
{
}

DrmShimPlane::DrmShimPlane(uint32_t planeId, HwcTestCrtc* crtc)
  : mPlaneId(planeId),
    mPlaneIx(0),
    mDsId(0),
    mCrtc(crtc),
    mRedrawExpected(false),
    mSetDisplayFailed(false),
    mBpp(0),    // No Bpp set on plane initially so 1st set plane will generate potential flicker
    mPixelFormat(0),
    mDrmCallStartTime(0),
    mBufferUpdated(false)
{
}

DrmShimPlane::~DrmShimPlane()
{
}

uint32_t DrmShimPlane::GetZOrder()
{
    HwcTestState* state = HwcTestState::getInstance();

    ALOG_ASSERT(state);

    if (state->GetDeviceType() == HwcTestState::eDeviceTypeBXT)
    {
        // On Broxton, Z-order is same as plane index
        return GetPlaneIndex();
    }
    else
    {
        // On BYT/CHT, plane Z-order is given by current LUT.
        HwcTestCrtc::SeqVector* seq = GetCrtc()->GetZOrder();
        if (seq)
        {
            if (GetPlaneIndex() < seq->size())
            {
                return (*seq)[GetPlaneIndex()];
            }
            else
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }
}

void DrmShimPlane::SetDisplayFrame(int32_t x, int32_t y, uint32_t w, uint32_t h)
{
    mTransform.SetDisplayOffset(x,y);

    if (HwcTestState::getInstance()->GetDeviceType() == HwcTestState::eDeviceTypeBXT)
    {
        mTransform.SetDisplayFrameSize(w, h);
    }
}

void DrmShimPlane::SetSourceCrop(float left, float top, float width, float height)
{
    HWCLOGD_COND(eLogDrm, "Plane %d SC (%f, %f) %fx%f",
        mPlaneId, double(left), double(top), double(width), double(height));
    mTransform.SetSourceCrop(left, top, width, height);
}

void DrmShimPlane::DrmCallStart()
{
    mDrmCallStartTime = systemTime(SYSTEM_TIME_MONOTONIC);
}

int64_t DrmShimPlane::GetDrmCallDuration()
{
    return systemTime(SYSTEM_TIME_MONOTONIC) - mDrmCallStartTime;
}

void DrmShimPlane::Flip()
{
    mFlippedBuffer = mTransform.GetBuf();
}

bool DrmShimPlane::IsUsing(android::sp<DrmShimBuffer> buf)
{
    return (mTransform.GetBuf() == buf) || (mFlippedBuffer == buf);
}

void DrmShimPlane::ProtectionCheck(android::sp<DrmShimBuffer>& buf)
{
    // Is protection consistent?
    bool bufferProtected = buf->IsReallyProtected();
    bool planeDecrypted = IsDecrypted();
    char strbuf[HWCVAL_DEFAULT_STRLEN];

    HWCCHECK(eCheckPavpConsistent);
    if (bufferProtected)
    {
        if (!planeDecrypted)
        {
            HWCERROR(eCheckPavpConsistent, "Plane %d %s protected but NOT decrypted",
                GetPlaneId(), buf->IdProtStr(strbuf));
        }
    }
    else
    {
        if (planeDecrypted)
        {
            HWCERROR(eCheckPavpConsistent, "Plane %d %s decrypted but NOT protected",
                GetPlaneId(), buf->IdProtStr(strbuf));
        }
    }
}

void DrmShimPlane::Expand(DrmShimSortedTransformVector& transforms)
{
    ATRACE_CALL();

    // Expand the FB currently set to this plane using what we know
    // of how each FB was composed
    char strbuf[HWCVAL_DEFAULT_STRLEN];
    android::sp<DrmShimBuffer> buf = mTransform.GetBuf();

    if (buf.get())
    {
        HWCLOGV_COND(eLogCombinedTransform, "Expanding plane %d (crtc %d @ %p) %s",GetPlaneId(),
            GetCrtc()->GetCrtcId(), GetCrtc(),
            buf->IdStr(strbuf));

        if (mCrtc->IsPanelFitterEnabled())
        {
            DrmShimTransform panelFittedTransform(mTransform, mCrtc->GetPanelFitterTransform());

            if (HWCCOND(eLogCombinedTransform))
            {
                panelFittedTransform.Log(ANDROID_LOG_VERBOSE, "Expanding using panel fitted transform");
            }

            buf->AddSourceFBsToList(transforms, panelFittedTransform);
        }
        else
        {
            buf->AddSourceFBsToList(transforms, mTransform);
        }

        if (!DidSetDisplayFail() && mCrtc->IsDisplayEnabled())
        {
            buf->SetUsed(true);
        }
    }
}

bool DrmShimPlane::FormatHasPixelAlpha()
{
    return ((mPixelFormat == DRM_FORMAT_ARGB8888) || (mPixelFormat == DRM_FORMAT_ABGR8888) ||
            (mPixelFormat == DRM_FORMAT_RGBA8888) || (mPixelFormat == DRM_FORMAT_BGRA8888));
}

void DrmShimPlane::ValidateFormat()
{
    if (GetZOrder() == 0)
    {
        HWCLOGV_COND(eLogDrm, "Pixel format at back of stack (plane %d) is 0x%x", mPlaneId, mPixelFormat);

        HWCCHECK(eCheckBackHwStackPixelFormat);
        if (FormatHasPixelAlpha())
        {
            HWCERROR(eCheckBackHwStackPixelFormat, "Plane at back of HW stack is RGBA/BGRA");
        }
    }
}

void DrmShimPlane::Log(int priority)
{
    HWCLOG(priority, "  Plane %d", mPlaneId);
    mTransform.Log(priority, "    ");
}

uint32_t DrmShimPlane::GetDrmPlaneId()
{
    // Gives plane id for SetDisplay structure.
    // When spoofing Nuclear on platforms which don't support it (e.g. CHV),
    // Universal Planes are only partially supported - so they will be
    // returned when Drm is queried, but they won't work in SetDisplay.
    // So we invent this concept of a "DrmPlaneId" which is the same as the
    // regular plane id on the sprite planes, but on the main plane we
    // instead use the CRTC Id.
#ifdef DRM_IOCTL_MODE_ATOMIC
    if (HwcTestState::getInstance()->IsOptionEnabled(eOptSpoofNuclear))
    {
        if (IsMainPlane())
        {
            return GetCrtc()->GetCrtcId();
        }
    }
#endif
    return mPlaneId;
}

// Tiling Related Definitions - this is what HWC uses to build on all platforms.
#ifndef I915_FORMAT_MOD_X_TILED
#define DRM_FORMAT_MOD_VENDOR_INTEL   0x01

#define fourcc_mod_code(vendor, val) \
((((uint64_t)DRM_FORMAT_MOD_VENDOR_## vendor) << 56) | (val & 0x00ffffffffffffffULL))

#define I915_FORMAT_MOD_X_TILED fourcc_mod_code(INTEL, 1)
#define I915_FORMAT_MOD_Y_TILED fourcc_mod_code(INTEL, 2)
#define I915_FORMAT_MOD_Yf_TILED fourcc_mod_code(INTEL, 3)
#endif

void DrmShimPlane::SetTilingFromModifier(__u64 modifier)
{
    switch (modifier)
    {
        case I915_FORMAT_MOD_X_TILED:
            mTiling = ePlaneXTiled;
            break;
        case I915_FORMAT_MOD_Y_TILED:
            mTiling = ePlaneYTiled;
            break;
        case I915_FORMAT_MOD_Yf_TILED:
            mTiling = ePlaneYfTiled;
            break;
        default:
            mTiling = ePlaneLinear;
            break;
    }
}
