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
#include "HwcTestTimeline.h"
#include "HwcTestState.h"
#include "HwcTestUtil.h"

#include <unistd.h>

HwcTestTimeline::HwcTestTimeline(uint32_t id)
    : mTimelineSeq(0), mCurrentSeq(0), mFencesClosed(0) {
  mTimeline = sw_sync_timeline_create();
  mName = android::String8::format("HwcTestTimeline %d", id);
}

HwcTestTimeline::~HwcTestTimeline() {
  HWCLOGI("HwcTestTimeline::~HwcTestTimeline()");
  CloseFence(mTimeline);
}

// Create a fence on the next timeline sequence
// Add it to the map, so we know which input fence it represents
int HwcTestTimeline::CreateShimFence(int inFence) {
  ++mTimelineSeq;

  int outFence = sw_sync_fence_create(mTimeline, mName.string(), mTimelineSeq);
  HWCLOGD_COND(eLogEventQueue,
               "%s::CreateShimFence inFence: %d Timeline: %d outFence: %d",
               mName.string(), inFence, mTimelineSeq, outFence);

  return outFence;
}

// Close the input fence.
// Keep a count so we know how much to advance the timeline by.
void HwcTestTimeline::CloseFence(int inFence) {
  HWCLOGV_COND(eLogEventQueue, "%s: Closed in fence %d", mName.string(),
               inFence);
  ++mFencesClosed;

  close(inFence);
}

// Find the shim fence mapping to the input fence and signal it.
// Also close the input fence which we assume is signalled.
void HwcTestTimeline::SignalShimFence() {
  int timelineIncrement = mFencesClosed - mCurrentSeq;

  if (timelineIncrement > 0) {
    HWCLOGD_COND(eLogEventQueue, "%s: Increment %d", mName.string(),
                 timelineIncrement);
    mCurrentSeq = mFencesClosed;
    sw_sync_timeline_inc(mTimeline, timelineIncrement);
  } else {
    HWCLOGD_COND(eLogEventQueue, "%s: NO INCREMENT!");
  }
}
