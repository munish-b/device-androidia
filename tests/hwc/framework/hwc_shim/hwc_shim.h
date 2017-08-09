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

#ifndef __HWC_SHIM_H__
#define __HWC_SHIM_H__

#include <dlfcn.h>
#include <cutils/log.h>
#include <assert.h>

extern "C" {
#include "hardware/hardware.h"
#include "hardware/hwcomposer2.h"
#include <hardware/gralloc.h>
}

#include <xf86drm.h>      //< For structs and types.
#include <xf86drmMode.h>  //< For structs and types.

#include "HwcTestDefs.h"
#include "HwcDrmShimCallback.h"
#include "HwcShimInitializer.h"

#include <utils/String8.h>
#include <utils/String16.h>
#include <utils/Vector.h>
#include <utils/SystemClock.h>
#include <binder/IServiceManager.h>

#include "HwcvalHwc1.h"

// Real Hwc service and log
//#include <HwcService.h>
//#include <IDiagnostic.h>

//#include "hwc_shim_binder.h"

class HwcShimService;
class HwcTestState;
class HwcShim;
class CHwcFrameControl;

struct ShimHwcProcs {
  HwcShim *shim;
};

class HwcShim : hwc2_device, public HwcShimInitializer {
 public:
  /// TODO set this form test
  /// Max call time
  uint64_t callTimeThreshold;
  /// Used to store time before call
  uint64_t callTimeStart;

 private:
  /// Initialize Hwc shim
  int HwcShimInit(void);
  int HwcShimInitDrivers(HwcTestState *state);

  /// Constructor
  HwcShim(const hw_module_t *module);
  /// Destructor
  virtual ~HwcShim();

  // For hooks into real HWC
  /// TODO used?
  void *mLibHwcHandle;

  /// Pointer to hwc device struct
  hwc2_device *hwc_composer_device;
  /// pointer to hw_dev handle to hwc
  hw_device_t *hw_dev;

  /// HWC1 interface to test kernel
  Hwcval::Hwc1 *mHwc1;

  /// Cast pointer to device struct to HwcShim class
  static HwcShim *GetComposerShim(hwc2_device *dev) {
    // ALOG_ASSERT(dev);
    return static_cast<HwcShim *>(dev);
  }

  /// Get drm display  info Displays
  void GetDisplayInfo(void);

  // Real HWC service
  //        android::sp<intel::vpg::hwc::IService> mHwcService;

  // Call time functions to check the call time of real HWC and drm
  // functions
  /// Record time before function call
  void StartCallTime(void);
  /// Check length of call time
  void EndCallTime(const char *function);

 public:
  /// Implementation of Real HWC - HookOpen
  static int HookOpen(const hw_module_t *module, const char *name,
                      hw_device_t **hw_device);
  /// Implementation of Real HWC - HookClose
  static int HookClose(struct hw_device_t *device);

  /// Implementation of Real HWC - OnEventControl
  int OnEventControl(int disp, int event, int enabled);

  int EnableVSync(int disp, bool enable);

 public:
  /// Complete initialization of shim in DRM mode
  virtual void HwcShimInitDrm(void);

 private:
  template <typename PFN, typename T>
  static hwc2_function_pointer_t ToHook(T function) {
    static_assert(std::is_same<PFN, T>::value, "Incompatible fn pointer");
    return reinterpret_cast<hwc2_function_pointer_t>(function);
  }

  template <typename T, int Y, typename... Args>
  static int32_t func_hook(hwc2_device_t *dev, Args... args) {
    HwcShim *temp1 = static_cast<HwcShim *>(dev);
    hwc2_device_t *hwc2_dvc =
        reinterpret_cast<hwc2_device_t *>(temp1->hwc_composer_device);
    T temp = reinterpret_cast<T>(hwc2_dvc->getFunction(hwc2_dvc, Y));
    return temp(hwc2_dvc, std::forward<Args>(args)...);
  }
  template <typename T, int Y, typename... Args>
  static void func_hookv(hwc2_device_t *dev, Args... args) {
    HwcShim *temp1 = static_cast<HwcShim *>(dev);
    hwc2_device_t *hwc2_dvc =
        reinterpret_cast<hwc2_device_t *>(temp1->hwc_composer_device);
    T temp = reinterpret_cast<T>(hwc2_dvc->getFunction(hwc2_dvc, Y));
    return temp(hwc2_dvc, std::forward<Args>(args)...);
  }
  template <typename T, int Y, typename... Args>
  static uint32_t func_hooku(hwc2_device_t *dev, Args... args) {
    HwcShim *temp1 = static_cast<HwcShim *>(dev);
    hwc2_device_t *hwc2_dvc =
        reinterpret_cast<hwc2_device_t *>(temp1->hwc_composer_device);
    T temp = reinterpret_cast<T>(hwc2_dvc->getFunction(hwc2_dvc, Y));
    return temp(hwc2_dvc, std::forward<Args>(args)...);
  }
  static hwc2_function_pointer_t HookDevGetFunction(struct hwc2_device *dev,
                                                    int32_t descriptor);

  /// Get pointer to functions in Real drm
  int GetFunctionPointer(void *LibHandle, const char *Symbol,
                         void **FunctionHandle, uint32_t debug);

  // Shim versions of HWC functions
  // function pointers for hwc functions
  /// Implementation of Real HWC - HookPreapre
  static int HookPrepare(struct hwc2_device *dev, size_t numDisplays,
                         hwcval_display_contents_t **displays);

  /// Implementation of Real HWC - HookSet
  static int HookSet(struct hwc2_device *dev, size_t numDisplays,
                     hwcval_display_contents_t **displays);

  /// Implementation of Real HWC - HooEventControl
  static int HookEventControl(struct hwc2_device *dev, int disp, int event,
                              int enabled);
  /// Implementation of Real HWC - HookBlank
  static int HookBlank(struct hwc2_device *dev, int disp, int blank);

  /// Implementation of Real HWC - HookQuery
  static int HookQuery(struct hwc2_device *dev, int what, int *value);

  /// Implementation of Real HWC - HookDump
  static void HookDump(struct hwc2_device *dev, char *buff, int buff_len);

  /// Implementation of Real HWC - HookGetDisplayConfigs
  static int HookGetDisplayConfigs(struct hwc2_device *dev, int disp,
                                   uint32_t *configs, size_t *numConfigs);

  /// Implementation of Real HWC - HookGetActiveConfig
  static int HookGetActiveConfig(struct hwc2_device *dev, int disp);

  /// Implementation of Real HWC - HookGetDisplayAtrtributes
  static int HookGetDisplayAttributes(struct hwc2_device *dev, int disp,
                                      uint32_t config,
                                      const uint32_t *attributes,
                                      int32_t *values);

  // function pointers to real HWC composer
  /// Implementation of Real HWC - OnPrepare
  int OnPrepare(size_t numDisplays, hwcval_display_contents_t **displays);

  /// Implementation of Real HWC - OnSet
  int OnSet(size_t numDisplays, hwcval_display_contents_t **displays);

  /// Implementation of Real HWC - OnBlank
  int OnBlank(int disp, int blank);

  /// Implementation of Real HWC - OnQuery
  int OnQuery(int what, int *value);

  /// Implementation of Real HWC - OnDump
  void OnDump(char *buff, int buff_len);

  /// Implementation of Real HWC - OnGetDisplayConfigs
  int OnGetDisplayConfigs(int disp, uint32_t *configs, size_t *numConfigs);

  /// Implementation of Real HWC - OnGetActiveConfig
  int OnGetActiveConfig(int disp);

  /// Implementation of Real HWC - OnGetDisplayAttributes
  int OnGetDisplayAttributes(int disp, uint32_t config,
                             const uint32_t *attributes, int32_t *values);

 private:
  ShimHwcProcs mShimProcs;
  HwcDrmShimCallback mDrmShimCallback;
};

#endif
