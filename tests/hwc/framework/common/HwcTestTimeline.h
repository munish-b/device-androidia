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
#ifndef __HwcTestTimeline_h__
#define __HwcTestTimeline_h__

#include "utils/String8.h"
#include "utils/KeyedVector.h"
#include "HwcTestDefs.h"

struct HwcTestTimelineData {
  int outFence;
  uint32_t timelineSeq;
};

class HwcTestTimeline {
 public:
  HwcTestTimeline(uint32_t id);
  virtual ~HwcTestTimeline();

  // Create a fence on the next timeline sequence
  // Add it to the map, so we know which input fence it represents
  int CreateShimFence(int inFence);

  // Close the input fence.
  // Find it in the map. If it's there, prepare to add one to the shim timeline
  // (but don't do it now as we want it to be atomic).
  // Remove the input fence from the map.
  void CloseFence(int inFence);

  // Signal all shim fences up to the last one where CloseFence was called on
  // the input fence.
  void SignalShimFence();

 private:
  // Private data
  volatile int mTimeline;          // Timeline handle
  volatile uint32_t mTimelineSeq;  // Timeline seq for fences we create
  uint32_t mCurrentSeq;            // Last signalled seq
  uint32_t mFencesClosed;          // Count of in fences closed
  android::String8 mName;
};

#endif  // __HwcTestTimeline_h__
