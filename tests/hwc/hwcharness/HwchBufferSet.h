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

#ifndef __HwchBufferSet_h__
#define __HwchBufferSet_h__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include <utils/Vector.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <ui/GraphicBuffer.h>
#include <hardware/hwcomposer2.h>

#include "HwchPattern.h"
#include "HwchDefs.h"

namespace Hwch {
class BufferSet : public android::RefBase {
 private:
  struct FencedBuffer {
    android::sp<android::GraphicBuffer> mBuf;
    int mReleaseFenceFd;
    uint32_t mParam;

    FencedBuffer(android::GraphicBuffer* buf = 0, int fenceFd = -1)
        : mBuf(buf),
          mReleaseFenceFd(fenceFd),
          mParam(HWCH_BUFFERPARAM_UNDEFINED) {
    }
  };

  uint32_t mNumBuffers;
  uint32_t mCurrentBuffer;
  uint32_t mNextBuffer;
  uint32_t mWidth;
  uint32_t mHeight;
  uint32_t mFormat;
  uint32_t mUsage;
  uint64_t mLastTimestamp;

  FencedBuffer* mFencedB;  // current buffer
  android::Vector<FencedBuffer> mBuffers;

  bool mBuffersUpdatedThisFrame;
  bool mBuffersFilledAtLeastOnce;

 public:
  BufferSet(uint32_t width, uint32_t height, uint32_t format,
            int32_t numBuffers = -1,
            uint32_t usage = GRALLOC_USAGE_HW_COMPOSER |
                             GRALLOC_USAGE_HW_TEXTURE |
                             GRALLOC_USAGE_HW_RENDER);
  ~BufferSet();

  android::sp<android::GraphicBuffer> Get();
  buffer_handle_t GetHandle();
  buffer_handle_t GetNextBuffer();
  bool NeedsUpdating();
  bool BuffersFilledAtLeastOnce();
  uint32_t& GetInstanceParam();
  bool SetNextBufferInstance(uint32_t index);
  void AdvanceTimestamp(uint64_t delta);
  void PostFrame(int fenceFd);

  void SetReleaseFence(int fenceFd);
  int WaitReleaseFence(uint32_t timeoutMs, const android::String8& str);
  void CloseAllFences();

  uint32_t GetWidth();
  uint32_t GetHeight();

  // Number of buffers so far created
  static uint32_t GetBufferCount();
};

class BufferSetPtr : public android::sp<Hwch::BufferSet> {
 public:
  virtual ~BufferSetPtr();
  BufferSetPtr& operator=(android::sp<Hwch::BufferSet> rhs);
};
};

inline uint32_t Hwch::BufferSet::GetWidth() {
  return mWidth;
}

inline uint32_t Hwch::BufferSet::GetHeight() {
  return mHeight;
}

inline bool Hwch::BufferSet::BuffersFilledAtLeastOnce() {
  if (!mBuffersFilledAtLeastOnce) {
    mBuffersFilledAtLeastOnce = true;
    return false;
  } else {
    return true;
  }
}

#endif  // __HwchBufferSet_h__
