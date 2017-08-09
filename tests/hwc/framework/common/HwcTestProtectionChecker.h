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
#ifndef __HwcTestProtectionChecker_h__
#define __HwcTestProtectionChecker_h__

// NOTE: HwcTestDefs.h sets defines which are used in the HWC and DRM stack.
// -> to be included before any other HWC or DRM header file.
#include "HwcTestDefs.h"
#include "HwcTestState.h"
#include "DrmShimBuffer.h"

#include <utils/KeyedVector.h>

class HwcTestKernel;

class HwcTestProtectionChecker {
 public:
  HwcTestProtectionChecker(HwcTestKernel* testKernel);

  Hwcval::ValidityType IsValid(const hwc_buffer_media_details_t& details,
                               uint32_t hwcFrame);

  // Called from the intercepted IVideoControl implementation
  void EnableEncryptedSession(uint32_t sessionID, uint32_t instanceID);
  void DisableEncryptedSessionEntry(uint32_t sessionID);
  void DisableEncryptedSessionExit(uint32_t sessionID, int64_t startTime,
                                   int64_t timestamp);
  void DisableAllEncryptedSessionsEntry();
  void DisableAllEncryptedSessionsExit();
  void SelfTeardown();
  void ExpectSelfTeardown();
  void ValidateSelfTeardownTime();
  void RestartSelfTeardown();
  void InvalidateOnModeChange();
  void TimestampChange();

 private:
  // Key is session ID
  // Value is instance ID
  struct InstanceInfo {
    uint32_t mInstance;
    Hwcval::ValidityType mValidity;

    const char* ValidityStr();
  };

  android::KeyedVector<uint32_t, InstanceInfo> mSessionInstances;
  Hwcval::FrameNums mTeardownFrame;
  int64_t mHotPlugTime;
  HwcTestKernel* mTestKernel;
};

#endif  // __HwcTestProtectionChecker_h__
