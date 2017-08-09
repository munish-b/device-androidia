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

#ifndef __BxService_h__
#define __BxService_h__

#include "iservice.h"
#include "HwcvalAbstractHwcServiceSubset.h"

#define IA_HWCREAL_SERVICE_NAME "hwc.info.real"

// BxService forwards most messages straight on
// to the real service.
class BxService : public android::BBinder, Hwcval::AbstractHwcServiceSubset {
 public:
  BxService();

  IBinder* onAsBinder();

  virtual android::status_t onTransact(uint32_t, const android::Parcel&,
                                       android::Parcel*, uint32_t);

  android::sp<hwcomposer::IService> Real();

 private:
  enum {
    // ==============================================
    // Public APIs - try not to reorder these

    GET_HWC_VERSION = IBinder::FIRST_CALL_TRANSACTION,

    // Dump options and current settings to logcat.
    DUMP_OPTIONS,

    // Override an option.
    SET_OPTION,

    // Enable hwclogviewer output to logcat
    ENABLE_LOG_TO_LOGCAT = 99,

    // accessor for IBinder interface functions
    TRANSACT_GET_DIAGNOSTIC = 100,
    TRANSACT_GET_DISPLAY_CONTROL,
    TRANSACT_GET_VIDEO_CONTROL
  };

  void GetRealService();
  android::sp<android::IBinder> mRealBinder;
  android::sp<hwcomposer::IService> mRealService;
};

#endif  // __BxService_h__
