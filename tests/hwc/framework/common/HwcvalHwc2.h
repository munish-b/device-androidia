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
#ifndef __Hwc2Iface_h__
#define __Hwc2Iface_h__

// NOTE: HwcTestDefs.h sets defines which are used in the HWC and DRM stack.
// -> to be included before any other HWC or DRM header file.
#include "HwcTestDefs.h"

#include "HwcTestKernel.h"
#include "DrmShimBuffer.h"
#include <utils/Vector.h>

#define EXPORT_API __attribute__((visibility("default")))

namespace Hwcval {
class EXPORT_API Hwc2 {
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
  Hwc2();

// Public interface used by the test

#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE
  // Shim of HWC IService
  EXPORT_API void SetHwcServiceShim(android::sp<HwcServiceShim> serviceShim);
#endif  // HWCVAL_BUILD_SHIM_HWCSERVICE

  //EXPORT_API void CheckValidateDisplayEntry(size_t numDisplays,
  //                                    hwcval_display_contents_t** displays);
 EXPORT_API void CheckValidateDisplayEntry();
  //EXPORT_API void CheckValidateDisplayExit(size_t numDisplays,
 //                                    hwcval_display_contents_t** displays);
  EXPORT_API void CheckValidateDisplayExit();

  /// Notify entry to PresentDisplay from HWC Shim
  //void EXPORT_API
  //    CheckPresentDisplayEnter(size_t numDisplays, hwcval_display_contents_t** displays);
  void EXPORT_API
      CheckPresentDisplayEnter(hwcval_display_contents_t** displays, hwc2_display_t display);

  /// Notify exit from PresentDisplay from HWC Shim, and perform checks
  //EXPORT_API void CheckPresentDisplayExit(size_t numDisplays,
  //                             hwcval_display_contents_t** displays);
  EXPORT_API void CheckPresentDisplayExit();

};
}

#endif  // __Hwc2Iface_h__
