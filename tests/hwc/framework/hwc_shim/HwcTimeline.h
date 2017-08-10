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

#ifndef INTEL_UFO_HWC_TIMELINE_H
#define INTEL_UFO_HWC_TIMELINE_H

#include "HwcTestState.h"

// ********************************************************************
// Drm Sync Timeline Class.
// This class provides a driver for buffer synchronisation via fence
// acquire/release.
// ********************************************************************

class HwcTimeline {
 public:
  // Fence handle.
  // NOTE:
  //   This must be a file descriptor handle type to support exchange between
  //   Android subsystems.
  //   Valid values are 0:N, -1 indicates NULL/invalid.
  typedef int Fence;

  HwcTimeline();
  ~HwcTimeline();

  const char* getName(void) {
    if (mpchName && strlen(mpchName))
      return mpchName;
    else
      return "N/A";
  }

  // One-time initialisation.
  // If initialisation is succesful then this will return true and the next
  // call to acquire( ) will block until a subsequent release( ).
  // Set pchName to a human-readable string that can be used to identify
  // this SyncTimeline and all the Fences created from it.
  // Pass explicit firstFutureTime to increase the initial delta between
  // acquire/release.
  // The SyncTimeline will be uninitialised on destruction.
  bool init(const char* pchName, uint32_t firstFutureTime = 1);

  // Uninitialise the timeline.
  void uninit(void);

  // Advance the 'future time' by N ticks.
  // Fences acquired after this is called will block until the future time is
  // reached.
  // By default, this will increase 'future time' by 1 tick.
  void advanceFutureTime(uint32_t delta = 1) {
    if (mSyncTimeline == -1) {
      HWCERROR(eCheckInternalError, "SyncTimeline is not initialised");
      return;
    }
    mNextFutureTime += delta;
    HWCLOGD("mSyncTimeline %d: mNextFutureTime %u", mSyncTimeline,
            mNextFutureTime);
  }

  // Returns the next 'future time'.
  uint32_t getFutureTime(void) {
    return mNextFutureTime;
  }

  // Returns the 'current time'.
  uint32_t getCurrentTime(void) {
    return mCurrentTime;
  }

  // Create a Fence that can be passed to another subsystem to block progress
  // until the future time is reached.
  // If an existing Fence is provided then a combined Fence that represents
  // completion of both is created to replace the existing Fence.
  // If succesful, returns true and fence will be updated.
  // The returned fence must be released using close( ).
  bool createFence(Fence* pFence);

  // Combines another fence into this existing fence, returning a fence that
  // represents completion of both.
  // Returns true if succesful - in which case pFence will be updated and
  // pOtherFence will be closed and reset to -1.
  // The returned fence must be released using close( ).
  static bool mergeFence(Fence* pFence, Fence* pOtherFence);

  // Duplicate an existing fence to this fence.
  // If an existing Fence is provided then a combined Fence that represents
  // completion of both is created to replace the existing Fence.
  // Returns true if succesful - in which case pFence will be updated.
  // The returned fence must be released using close( ).
  static bool dupFence(Fence* pFence, const Fence otherFence);

  // Advance the 'current time' by N ticks.
  // This will release all fences up to and including the new current time.
  // By default, this will increase time by 1 tick.
  void advance(uint32_t ticks = 1);
  void advanceTo(uint32_t absSync);

  // TODO:
  //  Add sync_fence_wait here
  /// int waitFence( .... ) { ... }

  // Dump trace for timeline status.
  void dumpTimeline(const char* pchPrefix = "");

  // Dump trace for a fence.
  static void dumpFence(Fence fence, const char* pchPrefix = "");

 protected:
  // Human-readable name for this sync timeline.
  char* mpchName;

  // Timeline handle.
  int mSyncTimeline;

  // Timeline 'current time' counter.
  uint32_t mCurrentTime;

  // This is the timeline 'future time'.
  // It is the fence counter used for any subsequent acquire( ) call.
  uint32_t mNextFutureTime;
};

#endif  // INTEL_UFO_HWC_TIMELINE_H
