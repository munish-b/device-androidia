/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "HwchBufferSet.h"
#include "HwchSystem.h"
#include "HwchDefs.h"
#include "HwcTestDefs.h"
#include "HwchBufferDestroyer.h"
#include "HwcTestState.h"
#include "HwcTestUtil.h"

extern "C" {  // shame
#include <intel_bufmgr.h>
#include <i915_drm.h>
#include <drm_fourcc.h>
}  // extern "C"


static uint32_t bufferCount = 0;

Hwch::BufferSet::BufferSet(uint32_t width, uint32_t height, uint32_t format,
                           int32_t numBuffers, uint32_t usage)
    : mCurrentBuffer(0),
      mNextBuffer(0),
      mWidth(width),
      mHeight(height),
      mFormat(format),
      mUsage(usage),
      mLastTimestamp(0),
      mBuffersUpdatedThisFrame(false),
      mBuffersFilledAtLeastOnce(false) {
  if (numBuffers < 0) {
    mNumBuffers = Hwch::System::getInstance().GetDefaultNumBuffers();
  } else {
    mNumBuffers = numBuffers;
  }

  HWCLOGV("BufferSet created @ %p, numBuffers=%d, usage=%x", this, mNumBuffers,
          usage);
  for (uint32_t i = 0; i < mNumBuffers; ++i) {
    android::GraphicBuffer* buf =
        new android::GraphicBuffer(width, height, format, usage);

    HWCLOGV("  Handle %p", buf->handle);
    FencedBuffer fb(buf, -1);
    mBuffers.add(fb);
  }
  GetNextBuffer();

  HWCLOGV_COND(eLogHarness, "Buffers allocated (C): %d",
               (bufferCount += mNumBuffers));

  if (bufferCount > 500) {
    HWCERROR(eCheckObjectLeak, "Buffers allocated: %d", bufferCount);
  }

  // Get shims to process the work queue
  HwcTestState::getInstance()->ProcessWork();
}

Hwch::BufferSet::~BufferSet() {
  HWCLOGV("BufferSet destroyed @ %p (%d buffers)", this, mNumBuffers);
  Hwch::System& system = Hwch::System::getInstance();
  for (uint32_t i = 0; i < mNumBuffers; ++i) {
    // Possibly should wait for the fence before releasing the buffer for
    // destruction
    // but this means we would need to hang on to the BufferSet for a while
    // after we've finished with it.
    // TODO!!
    FencedBuffer& fencedB = mBuffers.editItemAt(i);

    if (fencedB.mReleaseFenceFd != -1) {
      HWCLOGD_COND(eLogTimeline, "~BufferSet: Waiting for release fence %d",
                   fencedB.mReleaseFenceFd);
      mFencedB = &fencedB;
      android::String8 str =
          android::String8::format("Destroying %p", GetHandle());
      WaitReleaseFence(system.GetFenceTimeout(), str);
    }

    if (HwcTestState::getInstance()->IsOptionEnabled(
            eOptAsyncBufferDestruction)) {
      HWCLOGD("Defer destroying buffer handle %p until OnSet",
              fencedB.mBuf->handle);
      system.GetBufferDestroyer().Push(fencedB.mBuf);
    }
  }
  HWCLOGV_COND(eLogHarness, "Buffers allocated (~): %d",
               (bufferCount -= mNumBuffers));
}

bool Hwch::BufferSet::NeedsUpdating() {
  if (mBuffersUpdatedThisFrame) {
    return false;
  } else {
    mBuffersUpdatedThisFrame = true;
    return true;
  }
}

uint32_t& Hwch::BufferSet::GetInstanceParam() {
  return mFencedB->mParam;
}

bool Hwch::BufferSet::SetNextBufferInstance(uint32_t index) {
  while (index >= mNumBuffers) {
    HWCLOGD_COND(
        eLogHarness,
        "SetNextBufferInstance: new GraphicBuffer(%dx%d format %x usage %x",
        mWidth, mHeight, mFormat, mUsage);

    android::GraphicBuffer* buf =
        new android::GraphicBuffer(mWidth, mHeight, mFormat, mUsage);
    if ((!buf) || (buf->handle == NULL)) {
      HWCERROR(eCheckAllocFail,
               "SetNextBufferInstance gralloc allocation failure");
      return false;
    }

    FencedBuffer fb(buf, -1);
    mBuffers.add(fb);

    ++mNumBuffers;
  }

  mFencedB = &mBuffers.editItemAt(mCurrentBuffer);

  if (mNumBuffers > 1) {
    mBuffersUpdatedThisFrame = false;
  }

  mNextBuffer = index;

  return true;
}

buffer_handle_t Hwch::BufferSet::GetNextBuffer() {
  mFencedB = &mBuffers.editItemAt(mNextBuffer);
  mCurrentBuffer = mNextBuffer;
  mNextBuffer = (mNextBuffer + 1) % mNumBuffers;
  return mFencedB->mBuf->handle;
}

buffer_handle_t Hwch::BufferSet::GetHandle() {
  return mFencedB->mBuf->handle;
}

android::sp<android::GraphicBuffer> Hwch::BufferSet::Get() {
  return mFencedB->mBuf;
}
void Hwch::BufferSet::AdvanceTimestamp(uint64_t delta) {
  // intel::ufo::gralloc::GrallocClient& gralloc =
  // intel::ufo::gralloc::GrallocClient::getInstance();
  mLastTimestamp += delta;
#ifdef HWCVAL_GRALLOC_HAS_MEDIA_TIMESTAMPS
  gralloc.setBufferTimestamp(GetHandle(), mLastTimestamp);
#endif
}
void Hwch::BufferSet::PostFrame(int fenceFd) {
  // Don't allow rotation of buffers if only one buffer was allocated
  if (mNumBuffers > 1) {
    mBuffersUpdatedThisFrame = false;
  }
  SetReleaseFence(fenceFd);
}

void Hwch::BufferSet::SetReleaseFence(int fenceFd) {
  if (fenceFd > 0) {
    if (mFencedB->mReleaseFenceFd > 0) {
      int mergedFence = sync_merge("Hwch merged release fences",
                                   mFencedB->mReleaseFenceFd, fenceFd);
      HWCLOGD_COND(eLogTimeline,
                   "BufferSet: handle %p merged release fences (no change of "
                   "buffer) %d=%d+%d",
                   mFencedB->mBuf->handle, mergedFence,
                   mFencedB->mReleaseFenceFd, fenceFd);
      CloseFence(mFencedB->mReleaseFenceFd);
      CloseFence(fenceFd);
      mFencedB->mReleaseFenceFd = mergedFence;
    } else {
      HWCLOGD_COND(eLogTimeline, "BufferSet: handle %p has release fence %d",
                   mFencedB->mBuf->handle, fenceFd);
      mFencedB->mReleaseFenceFd = fenceFd;
    }
  }
}

int Hwch::BufferSet::WaitReleaseFence(uint32_t timeoutMs,
                                      const android::String8& str) {
  if (mFencedB->mReleaseFenceFd > 0) {
    int err = sync_wait(mFencedB->mReleaseFenceFd, 0);
    HWCCHECK(eCheckReleaseFenceTimeout);
    HWCCHECK(eCheckReleaseFenceWait);
    if (err < 0) {
      int64_t startWait = systemTime(SYSTEM_TIME_MONOTONIC);
      err = sync_wait(mFencedB->mReleaseFenceFd, timeoutMs);
      if (err < 0) {
        HWCERROR(eCheckReleaseFenceTimeout,
                 "Timeout waiting for release fence on layer %s handle %p",
                 str.string(), GetHandle());
      } else {
        float waitTime =
            float(systemTime(SYSTEM_TIME_MONOTONIC) - startWait) / 1000000.0;

        HWCERROR(
            eCheckReleaseFenceWait,
            "Wait %3.3fms required for release fence on layer %s handle %p",
            waitTime, str.string(), GetHandle());
      }
    }

    HWCLOGD_COND(eLogTimeline,
                 "BufferSet::WaitReleaseFence: Closing release fence %d",
                 mFencedB->mReleaseFenceFd);
    CloseFence(mFencedB->mReleaseFenceFd);
    mFencedB->mReleaseFenceFd = -1;
    return err;
  } else {
    return 0;
  }
}

void Hwch::BufferSet::CloseAllFences() {
  Hwch::System& system = Hwch::System::getInstance();
  WaitReleaseFence(system.GetFenceTimeout(),
                   android::String8("FRAMEBUFFER_TARGET(Closedown)"));

  for (uint32_t i = 0; i < mBuffers.size(); ++i) {
    FencedBuffer& fencedB = mBuffers.editItemAt(i);

    if (fencedB.mReleaseFenceFd != -1) {
      HWCLOGD_COND(eLogTimeline, "CloseAllFences: Closing release fence %d",
                   fencedB.mReleaseFenceFd);
      CloseFence(fencedB.mReleaseFenceFd);
      fencedB.mReleaseFenceFd = -1;
    }
  }
}

Hwch::BufferSetPtr::~BufferSetPtr() {
  // Force operator= code to happen
  *this = 0;
}

Hwch::BufferSetPtr& Hwch::BufferSetPtr::operator=(
    android::sp<Hwch::BufferSet> rhs) {
  if (rhs.get() != get()) {
    android::sp<BufferSet>& bufs =
        *(static_cast<android::sp<BufferSet>*>(this));
    Hwch::System::getInstance().RetainBufferSet(bufs);
    bufs = rhs;
  }

  return *this;
}

uint32_t Hwch::BufferSet::GetBufferCount() {
  return bufferCount;
}
