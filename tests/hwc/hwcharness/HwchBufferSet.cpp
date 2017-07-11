/****************************************************************************
*
* Copyright (c) Intel Corporation (2014).
*
* DISCLAIMER OF WARRANTY
* NEITHER INTEL NOR ITS SUPPLIERS MAKE ANY REPRESENTATION OR WARRANTY OR
* CONDITION OF ANY KIND WHETHER EXPRESS OR IMPLIED (EITHER IN FACT OR BY
* OPERATION OF LAW) WITH RESPECT TO THE SOURCE CODE.  INTEL AND ITS SUPPLIERS
* EXPRESSLY DISCLAIM ALL WARRANTIES OR CONDITIONS OF MERCHANTABILITY OR
* FITNESS FOR A PARTICULAR PURPOSE.  INTEL AND ITS SUPPLIERS DO NOT WARRANT
* THAT THE SOURCE CODE IS ERROR-FREE OR THAT OPERATION OF THE SOURCE CODE WILL
* BE SECURE OR UNINTERRUPTED AND HEREBY DISCLAIM ANY AND ALL LIABILITY ON
* ACCOUNT THEREOF.  THERE IS ALSO NO IMPLIED WARRANTY OF NON-INFRINGEMENT.
* SOURCE CODE IS LICENSED TO LICENSEE ON AN "AS IS" BASIS AND NEITHER INTEL
* NOR ITS SUPPLIERS WILL PROVIDE ANY SUPPORT, ASSISTANCE, INSTALLATION,
* TRAINING OR OTHER SERVICES.  INTEL AND ITS SUPPLIERS WILL NOT PROVIDE ANY
* UPDATES, ENHANCEMENTS OR EXTENSIONS.
*
* File Name:            HwchBufferSet.cpp
*
* Description:          Buffer Set class implementation
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#include "HwchBufferSet.h"
#include "HwchSystem.h"
#include "HwchDefs.h"
#include "HwcTestDefs.h"
#include "HwchBufferDestroyer.h"
#include "HwcTestState.h"
#include "HwcTestUtil.h"

#include "GrallocClient.h"

extern "C" { // shame
    #include <intel_bufmgr.h>
    #include <i915_drm.h>
    #include <drm_fourcc.h>
} // extern "C"

// Shift/mask to extract PAVP Session instance from the 32bit datatype.
#define DRM_BO_DATATYPE_INSTANCE_SHIFT   16
#define DRM_BO_DATATYPE_INSTANCE_MASK    0xFFFF

#ifdef HWCVAL_BUILD_PAVP
#if defined(DRM_IOCTL_I915_GEM_ACCESS_DATATYPE)
#define DRM_ACCESS_USERDATA DRM_IOCTL_I915_GEM_ACCESS_DATATYPE
#elif defined(DRM_IOCTL_I915_GEM_ACCESS_USERDATA)
#define DRM_ACCESS_USERDATA DRM_IOCTL_I915_GEM_ACCESS_USERDATA
#else
#error "Missing DRM_IOCTL_I915_GEM_ACCESS_DATATYPE and DRM_IOCTL_I915_GEM_ACCESS_USERDATA"
#endif
#endif // HWCVAL_BUILD_PAVP

static uint32_t bufferCount = 0;

Hwch::BufferSet::BufferSet(uint32_t width, uint32_t height, uint32_t format, int32_t numBuffers, uint32_t usage)
  : mCurrentBuffer(0),
    mNextBuffer(0),
    mWidth(width),
    mHeight(height),
    mFormat(format),
    mUsage(usage),
    mLastTimestamp(0),
    mEncrypted(false),
    mBuffersUpdatedThisFrame(false),
    mBuffersFilledAtLeastOnce(false)
{
    if (numBuffers < 0)
    {
        mNumBuffers = Hwch::System::getInstance().GetDefaultNumBuffers();
    }
    else
    {
        mNumBuffers = numBuffers;
    }

    HWCLOGV("BufferSet created @ %p, numBuffers=%d, usage=%x", this, mNumBuffers, usage);
    for (uint32_t i=0; i<mNumBuffers; ++i)
    {
        android::GraphicBuffer* buf = new android::GraphicBuffer(width, height, format, usage);

        HWCLOGV("  Handle %p", buf->handle);
        FencedBuffer fb(buf, -1);
        mBuffers.add(fb);
    }
    GetNextBuffer();

    HWCLOGV_COND(eLogHarness, "Buffers allocated (C): %d", (bufferCount += mNumBuffers));

    if (bufferCount > 500)
    {
        HWCERROR(eCheckObjectLeak, "Buffers allocated: %d", bufferCount);
    }

    // Get shims to process the work queue
    HwcTestState::getInstance()->ProcessWork();
}

Hwch::BufferSet::~BufferSet()
{
    HWCLOGV("BufferSet destroyed @ %p (%d buffers)",this, mNumBuffers);
    Hwch::System& system = Hwch::System::getInstance();
    for (uint32_t i=0; i<mNumBuffers; ++i)
    {
        // Possibly should wait for the fence before releasing the buffer for destruction
        // but this means we would need to hang on to the BufferSet for a while after we've finished with it.
        // TODO!!
        FencedBuffer& fencedB = mBuffers.editItemAt(i);

        if (fencedB.mReleaseFenceFd != -1)
        {
            HWCLOGD_COND(eLogTimeline, "~BufferSet: Waiting for release fence %d", fencedB.mReleaseFenceFd);
            mFencedB = &fencedB;
            android::String8 str = android::String8::format("Destroying %p", GetHandle());
            WaitReleaseFence(system.GetFenceTimeout(), str);
        }

        if (HwcTestState::getInstance()->IsOptionEnabled(eOptAsyncBufferDestruction))
        {
            HWCLOGD("Defer destroying buffer handle %p until OnSet", fencedB.mBuf->handle);
            system.GetBufferDestroyer().Push(fencedB.mBuf);
        }
    }
    HWCLOGV_COND(eLogHarness, "Buffers allocated (~): %d", (bufferCount -= mNumBuffers));
}

bool Hwch::BufferSet::NeedsUpdating()
{
    if (mBuffersUpdatedThisFrame)
    {
        return false;
    }
    else
    {
        mBuffersUpdatedThisFrame = true;
        return true;
    }
}

uint32_t& Hwch::BufferSet::GetInstanceParam()
{
    return mFencedB->mParam;
}

bool Hwch::BufferSet::SetNextBufferInstance(uint32_t index)
{
    while (index >= mNumBuffers)
    {
        HWCLOGD_COND(eLogHarness, "SetNextBufferInstance: new GraphicBuffer(%dx%d format %x usage %x", mWidth, mHeight, mFormat, mUsage);

        android::GraphicBuffer* buf = new android::GraphicBuffer(mWidth, mHeight, mFormat, mUsage);
        if ((!buf) || (buf->handle == NULL))
        {
            HWCERROR(eCheckAllocFail, "SetNextBufferInstance gralloc allocation failure");
            return false;
        }

        FencedBuffer fb(buf, -1);
        mBuffers.add(fb);

#ifdef HWCVAL_BUILD_PAVP
        intel::ufo::gralloc::GrallocClient& gralloc = intel::ufo::gralloc::GrallocClient::getInstance();
        gralloc.setBufferPavpSession(buf->handle, mSessionId, mInstance, mEncrypted);
#endif

        ++mNumBuffers;
    }

    mFencedB = &mBuffers.editItemAt(mCurrentBuffer);

    if (mNumBuffers > 1)
    {
        mBuffersUpdatedThisFrame = false;
    }

    mNextBuffer = index;

    return true;
}

buffer_handle_t Hwch::BufferSet::GetNextBuffer()
{
    mFencedB = &mBuffers.editItemAt(mNextBuffer);
    mCurrentBuffer = mNextBuffer;
    mNextBuffer = (mNextBuffer+1) % mNumBuffers;
    return mFencedB->mBuf->handle;
}

buffer_handle_t Hwch::BufferSet::GetHandle()
{
    return mFencedB->mBuf->handle;
}

android::sp<android::GraphicBuffer> Hwch::BufferSet::Get()
{
    return mFencedB->mBuf;
}

void Hwch::BufferSet::AdvanceTimestamp(uint64_t delta)
{
    intel::ufo::gralloc::GrallocClient& gralloc = intel::ufo::gralloc::GrallocClient::getInstance();
    mLastTimestamp += delta;
#ifdef HWCVAL_GRALLOC_HAS_MEDIA_TIMESTAMPS
    gralloc.setBufferTimestamp(GetHandle(), mLastTimestamp);
#endif
}

void Hwch::BufferSet::PostFrame(int fenceFd)
{
    // Don't allow rotation of buffers if only one buffer was allocated
    if (mNumBuffers > 1)
    {
        mBuffersUpdatedThisFrame = false;
    }
    SetReleaseFence(fenceFd);
}

void Hwch::BufferSet::SetReleaseFence(int fenceFd)
{
    if (fenceFd > 0)
    {
        if (mFencedB->mReleaseFenceFd > 0)
        {
            int mergedFence = sync_merge("Hwch merged release fences", mFencedB->mReleaseFenceFd, fenceFd);
            HWCLOGD_COND(eLogTimeline, "BufferSet: handle %p merged release fences (no change of buffer) %d=%d+%d",
                mFencedB->mBuf->handle, mergedFence, mFencedB->mReleaseFenceFd, fenceFd);
            CloseFence(mFencedB->mReleaseFenceFd);
            CloseFence(fenceFd);
            mFencedB->mReleaseFenceFd = mergedFence;
        }
        else
        {
            HWCLOGD_COND(eLogTimeline, "BufferSet: handle %p has release fence %d", mFencedB->mBuf->handle, fenceFd);
            mFencedB->mReleaseFenceFd = fenceFd;
        }
    }

}

int Hwch::BufferSet::WaitReleaseFence(uint32_t timeoutMs, const android::String8& str)
{
    if (mFencedB->mReleaseFenceFd > 0)
    {
        int err = sync_wait(mFencedB->mReleaseFenceFd, 0);
        HWCCHECK(eCheckReleaseFenceTimeout);
        HWCCHECK(eCheckReleaseFenceWait);
        if (err < 0)
        {
            int64_t startWait = systemTime(SYSTEM_TIME_MONOTONIC);
            err = sync_wait(mFencedB->mReleaseFenceFd, timeoutMs);
            if (err < 0)
            {
                HWCERROR(eCheckReleaseFenceTimeout,
                         "Timeout waiting for release fence on layer %s handle %p",
                         str.string(), GetHandle());
            }
            else
            {
                float waitTime = float(systemTime(SYSTEM_TIME_MONOTONIC) - startWait) / 1000000.0;

                HWCERROR(eCheckReleaseFenceWait,
                        "Wait %3.3fms required for release fence on layer %s handle %p",
                        waitTime,
                        str.string(), GetHandle());
            }
        }

        HWCLOGD_COND(eLogTimeline, "BufferSet::WaitReleaseFence: Closing release fence %d", mFencedB->mReleaseFenceFd);
        CloseFence(mFencedB->mReleaseFenceFd);
        mFencedB->mReleaseFenceFd = -1;
        return err;
    }
    else
    {
        return 0;
    }
}

void Hwch::BufferSet::CloseAllFences()
{
    Hwch::System& system = Hwch::System::getInstance();
    WaitReleaseFence(system.GetFenceTimeout(), android::String8("FRAMEBUFFER_TARGET(Closedown)"));

    for (uint32_t i=0; i<mBuffers.size(); ++i)
    {
        FencedBuffer& fencedB = mBuffers.editItemAt(i);

        if (fencedB.mReleaseFenceFd != -1)
        {
            HWCLOGD_COND(eLogTimeline, "CloseAllFences: Closing release fence %d", fencedB.mReleaseFenceFd);
            CloseFence (fencedB.mReleaseFenceFd);
            fencedB.mReleaseFenceFd = -1;
        }
    }
}

#ifdef HWCVAL_BUILD_PAVP
void Hwch::BufferSet::SetProtectionState(bool encrypted)
{
    Hwch::System& system = Hwch::System::getInstance();
    SetProtectionState(encrypted, system.GetPavpSessionId(), system.GetPavpInstance());
}

void Hwch::BufferSet::SetProtectionState(bool encrypted, uint32_t sessionId, uint32_t instance)
{
    mEncrypted = encrypted;
    intel::ufo::gralloc::GrallocClient& gralloc = intel::ufo::gralloc::GrallocClient::getInstance();

    for (uint32_t i=0; i<mBuffers.size(); ++i)
    {
        FencedBuffer& fencedB = mBuffers.editItemAt(i);

        gralloc.setBufferPavpSession(fencedB.mBuf->handle, sessionId, instance, encrypted);
    }

    // Cache the session and instance ids for use in Replays
    mSessionId = sessionId;
    mInstance = instance;
}
#endif

Hwch::BufferSetPtr::~BufferSetPtr()
{
    // Force operator= code to happen
    *this = 0;
}

Hwch::BufferSetPtr& Hwch::BufferSetPtr::operator=(android::sp<Hwch::BufferSet> rhs)
{
    if (rhs.get() != get())
    {
        android::sp<BufferSet>& bufs = *(static_cast<android::sp<BufferSet>* > (this));
        Hwch::System::getInstance().RetainBufferSet(bufs);
        bufs = rhs;
    }

    return *this;
}

uint32_t Hwch::BufferSet::GetBufferCount()
{
    return bufferCount;
}


