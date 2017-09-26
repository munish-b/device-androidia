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
#ifndef __Hwc1Iface_h__
#define __Hwc1Iface_h__

// NOTE: HwcTestDefs.h sets defines which are used in the HWC and DRM stack.
// -> to be included before any other HWC or DRM header file.
#include "HwcTestDefs.h"

#include "HwcTestKernel.h"
#include "DrmShimBuffer.h"
#include <utils/Vector.h>

#define EXPORT_API __attribute__((visibility("default")))

namespace Hwcval {
class EXPORT_API Hwc1 {
 private:
  /// Pointer to test state
  HwcTestState* mState;
  HwcTestKernel* mTestKernel;

  // Layer validity is stored separately as it is required in onPrepare
  android::Vector<Hwcval::ValidityType> mLayerValidity[HWCVAL_MAX_CRTCS];

  // Current layer lists in the main thread
  Hwcval::LayerList* mContent[HWCVAL_MAX_CRTCS];

  // OnSet sequence number for validation
  uint32_t mHwcFrame;

  // Number of displays with content in OnSet
  uint32_t mActiveDisplays;

 public:
  //-----------------------------------------------------------------------------
  // Constructor & Destructor
  Hwc1();

// Public interface used by the test

#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE
  // Shim of HWC IService
  EXPORT_API void SetHwcServiceShim(android::sp<HwcServiceShim> serviceShim);
#endif  // HWCVAL_BUILD_SHIM_HWCSERVICE

  EXPORT_API void CheckOnPrepareEntry(size_t numDisplays,
                                      hwcval_display_contents_t** displays);
  EXPORT_API void CheckOnPrepareExit(size_t numDisplays,
                                     hwcval_display_contents_t** displays);

  /// Notify entry to onSet from HWC Shim
  void EXPORT_API
      CheckSetEnter(size_t numDisplays, hwcval_display_contents_t** displays);

  /// Notify exit from OnSet from HWC Shim, and perform checks
  EXPORT_API void CheckSetExit(size_t numDisplays,
                               hwcval_display_contents_t** displays);

  /// Checks before HWC is requested to blank the display
  EXPORT_API void CheckBlankEnter(int disp, int blank);
  /// Checks after HWC is requested to blank the display
  EXPORT_API void CheckBlankExit(int disp, int blank);

  // Display config checking
  EXPORT_API void GetDisplayConfigsExit(int disp, uint32_t* configs,
                                        uint32_t numConfigs);
  EXPORT_API void GetActiveConfigExit(uint32_t disp, uint32_t config);
  EXPORT_API void GetDisplayAttributesExit(uint32_t disp, uint32_t config,
                                           const int32_t attribute,
                                           int32_t* values);
};
}

#endif  // __Hwc1Iface_h__
