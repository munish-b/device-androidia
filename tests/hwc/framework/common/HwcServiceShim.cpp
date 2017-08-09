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
#include "HwcServiceShim.h"

#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE

#include <binder/IInterface.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include "HwcTestKernel.h"
#include "HwcvalServiceManager.h"
#include "HwcTestMdsControl.h"
#include "HwcTestDisplayControl.h"

HwcServiceShim::HwcServiceShim() {
  HWCLOGV("HwcServiceShim created @%p", this);
  memset(mDisplayControls, 0, sizeof(mDisplayControls));
}

HwcServiceShim::~HwcServiceShim() {
}

bool HwcServiceShim::Start() {
  ALOGD("Starting %s in shim", IA_HWC_SERVICE_NAME);
  if (hwcvalServiceManager()->OverrideService(String16(IA_HWC_SERVICE_NAME),
                                              String16(IA_HWCREAL_SERVICE_NAME),
                                              this, false)) {
    HWCERROR(eCheckHwcServiceBind, "Failed to start HWC Shim Service (%s)",
             IA_HWC_SERVICE_NAME);
    return false;
  }
  HWCLOGA("Started %s in shim", IA_HWC_SERVICE_NAME);
  return true;
}

const android::String16 HwcServiceShim::descriptor("IA.IService.Shim");
const android::String16& HwcServiceShim::getInterfaceDescriptor() const {
  return descriptor;
}

sp<IDisplayControl> HwcServiceShim::getDisplayControl(uint32_t display) {
  if (mDisplayControls[display] == 0) {
    HWCLOGD("HwcServiceShim::getDisplayControl(%d) creating display control",
            display);
    sp<IDisplayControl> realDispControl =
        NULL;  // Real()->GetDisplayControl(display);
    HwcTestKernel* testKernel = HwcTestState::getInstance()->GetTestKernel();

    ALOG_ASSERT(realDispControl.get());
    ALOG_ASSERT(testKernel);
    // sp<IDisplayControl> dispControl = new HwcTestDisplayControl(display,
    // realDispControl, testKernel);
    // imDisplayControls[display] = dispControl;
  }

  return mDisplayControls[display];
}

sp<IVideoControl> HwcServiceShim::getVideoControl() {
  sp<IVideoControl> video = NULL;  // Real()->GetVideoControl();
  HwcTestKernel* testKernel = HwcTestState::getInstance()->GetTestKernel();

  if (testKernel) {
    // We are shimming the IVideoControl
    if (mVideoControl.get() == 0) {
      // mVideoControl = new HwcTestVideoControl(video, testKernel);
    }

    return mVideoControl;
  } else {
    // Not shimming (shims not installed)
    return video;
  }
}

#endif  // HWCVAL_BUILD_SHIM_HWCSERVICE
