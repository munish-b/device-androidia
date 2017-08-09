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

#ifndef __TEST_BINDER_H__
#define __TEST_BINDER_H__

#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include <utils/String8.h>
#include <utils/Log.h>

#include "IHwcShimService.h"

class HwcTestConfig;
class HwcTestResult;

using namespace android;

namespace android {

class BpHwcShimService : public BpInterface<IHwcShimService> {
 public:
  /// TODO comment
  BpHwcShimService(const sp<IBinder>& impl);

  ~BpHwcShimService();

  /// Get result of all checks
  virtual status_t GetHwcTestResult(HwcTestResult& result,
                                    bool disableAllChecks);
  /// Set test configuration including all check enables
  virtual status_t SetHwcTestConfig(const HwcTestConfig& config,
                                    bool resetResult);
  /// Get test configuration
  virtual status_t GetHwcTestConfig(HwcTestConfig& config);
};
}
#endif  // __TEST_BINDER_H__
