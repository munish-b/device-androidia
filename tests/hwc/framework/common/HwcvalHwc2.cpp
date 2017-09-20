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

#include <unistd.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <math.h>

#include "HwcTestState.h"
#include "HwcvalHwc2.h"
#include "HwcvalHwc1Content.h"
#include "HwcvalThreadTable.h"
#include "HwcvalStall.h"

#undef LOG_TAG
#define LOG_TAG "DRM_SHIM"

using namespace Hwcval;

/// Constructor
Hwcval::Hwc2::Hwc2()
    : mState(HwcTestState::getInstance()), mHwcFrame((uint32_t)-1) {
  mTestKernel = mState->GetTestKernel();
}

EXPORT_API void Hwcval::Hwc2::CheckValidateDisplayEntry() {
  // TODO Adding numDisplays as local var, enumerate correctly in future
  uint32_t numDisplays = 3;
  // HWC frame number
  ++mHwcFrame;
  bool divergeFrameNumbers = mState->IsOptionEnabled(eOptDivergeFrameNumbers);

  for (uint32_t displayIx = 0; displayIx < numDisplays; ++displayIx) {
    if (!divergeFrameNumbers) {
      mTestKernel->AdvanceFrame(displayIx, mHwcFrame);
    }
    if (divergeFrameNumbers) {
      mTestKernel->AdvanceFrame(displayIx);
    }
  }
}

EXPORT_API void Hwcval::Hwc2::CheckValidateDisplayExit() {
  HWCLOGD("In CheckOnPrepareExit");
}

/// Called by HWC Shim to notify that Present Display has occurred, and pass
/// in
/// the
/// contents of the display structures
EXPORT_API void Hwcval::Hwc2::CheckPresentDisplayEnter(hwcval_display_contents_t** displays, hwc2_display_t display) {
}

void Hwcval::Hwc2::CheckPresentDisplayExit() {
}
