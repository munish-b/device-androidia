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

#ifndef __IHwcShimService_h__
#define __IHwcShimService_h__

#include <binder/IInterface.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>

#include <utils/String8.h>

class HwcTestConfig;
class HwcTestResult;

namespace android {

class IHwcShimService : public IInterface {
 public:
  /// binder transaction types
  enum {
    eALERT = IBinder::FIRST_CALL_TRANSACTION,
    eSET_HWC_TEST_CONFIG,
    eGET_HWC_TEST_CONFIG,
    eGET_HWC_TEST_RESULT
  };

  /// Get result of all checks
  virtual status_t GetHwcTestResult(HwcTestResult& result,
                                    bool disableAllChecks) = 0;
  /// Set test configuration including all check enables
  virtual status_t SetHwcTestConfig(const HwcTestConfig& config,
                                    bool resetResult) = 0;
  /// Get test configuration
  virtual status_t GetHwcTestConfig(HwcTestConfig& config) = 0;

  /// Android macro
  DECLARE_META_INTERFACE(HwcShimService);
};
}

#endif  // _HWC_SHIM_SERVICE_H__
