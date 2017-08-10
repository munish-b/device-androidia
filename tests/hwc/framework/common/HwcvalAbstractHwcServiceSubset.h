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

#ifndef __HwcvalAbstractHwcServiceSubset_h__
#define __HwcvalAbstractHwcServiceSubset_h__

#include <binder/IInterface.h>
#include <binder/Parcel.h>

namespace android {
class String8;
};

namespace hwcomposer {

class IDiagnostic;
class IDisplayControl;
class IVideoControl;

}  // namespace services

namespace Hwcval {

/** Maintenance interface to control HWC activity.
 */
class AbstractHwcServiceSubset : public android::IInterface {
 public:
  virtual android::sp<hwcomposer::IDisplayControl> getDisplayControl(
      uint32_t display) = 0;
  virtual android::sp<hwcomposer::IVideoControl> getVideoControl() = 0;
};

}  // namespace Hwcval

#endif  // __HwcvalAbstractHwcServiceSubset_h__
