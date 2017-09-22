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

#undef LOG_TAG
#define LOG_TAG "HWC_SHIM"

#include <stdlib.h>
#include <dlfcn.h>
#include <cutils/properties.h>

#include "HwcTestDefs.h"

#include "hardware/hwcomposer_defs.h"

#include "HwcTestState.h"
#include "hwc_shim_binder.h"

#include "DrmShimChecks.h"
#include "HwcTestConfig.h"
#include "HwcTestUtil.h"
#include "HwcShimInitializer.h"
#include "HwcvalServiceManager.h"
#include "HwcServiceShim.h"
#include "HwcvalThreadTable.h"

// HwcTestState Constructor
HwcTestState::HwcTestState()
    : mDrmChecks(0),
      mTestKernel(0),
      mHwcShimInitializer(0),
      mLive(true),
      mHwcShimService(0),
      mRunningShims(0),
      mpHwcLogAdd(0),
      mpHwcSimulateHotPlug(0),
      mHotPlugInProgress(0),
      mNewDisplayConnectionState(true),
      mFrameControlEnabled(false),
      mFirstDisplayWithVSync(0),
      mWidiVisualisationHandle(0),
      mDisplaySpoof(&mDefaultDisplaySpoof),
      mVSyncRestorer(0),
      mOnSetConditionEnable(false),
      mMDSInfoProviderShim(0),
      mDeviceType(eDeviceTypeUnknown),
      mfpdrm_intel_bo_map(0),
      mfpdrm_intel_bo_unmap(0),
      mMaxDumpImages(0),
      mNumDumpImage(0),
      mMaxUnblankingLatency(HWCVAL_MAX_UNBLANKING_LATENCY_DEFAULT_US) {
  Hwcval::InitThreadStates();
  SetShimFail(eCheckSFRestarted);
}

HwcTestState::~HwcTestState() {
  HWCLOGI("Destroying HwcTestState...");
  delete mTestKernel;
  HWCLOGI("...Destroyed HwcTestState");
  mInstance = 0;
}

HwcTestState* HwcTestState::mInstance = 0;

HwcTestState* HwcTestState::getInstance() {
  if (mInstance == 0) {
    mInstance = new HwcTestState();
  }
  return mInstance;
}

void HwcTestState::rundown() {
  static volatile int sStateDeleted = 0;
  HWCLOGE("HwcTestState::rundown() - which means SF is exiting");

  if (android_atomic_swap(1, &sStateDeleted) == 0) {
    delete mInstance;
  }
}

void HwcTestState::CreateTestKernel() {
  mDrmChecks = new DrmShimChecks();
  mTestKernel = mDrmChecks;
  SetPreferences();
}

int HwcTestState::LoggingInit(void* libHwcHandle) {
  mLibHwcHandle = libHwcHandle;
  const char* sym;

  dlerror();
  sym = "hwcLogAdd";
  mpHwcLogAdd = (HwcLogAddPtr)dlsym(mLibHwcHandle, sym);

  if (mpHwcLogAdd) {
    (*mpHwcLogAdd)("HWC Shim connected to HWCLogAdd");
    HWCLOGI("HWC Shim connected to HWCLogAdd");
  } else {
    HWCLOGI("HWC Shim failed to connect to HWCLogAdd");
  }

  CreateTestKernel();

  return 0;
}

// Load HWC library and get hooks
// TODO move everything that can occur at construction time to the ctor
// use this for post construction settings from the test maybe rename.
int HwcTestState::TestStateInit(HwcShimInitializer* hwcShimInitializer) {
  const char* sym;

  // TODO turn off some logging check android levels
  HWCLOGI("In HwcTestState Init");

  mHwcShimInitializer = hwcShimInitializer;

  dlerror();
  sym = "hwcSimulateHotPlug";
  mpHwcSimulateHotPlug = (HwcSimulateHotPlugPtr)dlsym(mLibHwcHandle, sym);

  if (mpHwcSimulateHotPlug) {
    HWCLOGI("HWC has hotplug simulation facility");
  } else {
    HWCLOGI("HWC does not have hotplug simulation facility");
  }

#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE
  // Start HWC service shim
  mHwcServiceShim = new HwcServiceShim();
  mHwcServiceShim->Start();
#endif  // HWCVAL_BUILD_SHIM_HWCSERVICE

  // Create DRM Shim Checker Object, and give a pointer to the DRM Shim
  mTestKernel = mDrmChecks;

#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE
  mTestKernel->SetHwcServiceShim(mHwcServiceShim);
#endif  // HWCVAL_BUILD_SHIM_HWCSERVICE

  // Set preferences from the environment
  SetPreferences();

// Start service
#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE
  mHwcShimService = new HwcShimService(this);
#endif  // HWCVAL_BUILD_SHIM_HWCSERVICE

  atexit(HwcTestState::rundown);

  return 0;
}

void HwcTestState::RegisterWithHwc() {
}

void HwcTestState::SetPreferences() {
  char modeStr[PROPERTY_VALUE_MAX];
  if (property_get("hwcval.preferred_hdmi_mode", modeStr, NULL)) {
    // Mode format is <width>x<height>:<refresh rate>
    // You can use 0 for any value where you don't care.
    HWCLOGI("Processing hwcval.preferred_hdmi_mode=%s", modeStr);
    const char* p = modeStr;
    uint32_t w = atoiinc(p);
    if (*p++ != 'x') {
      return;
    }

    uint32_t h = atoiinc(p);
    if (*p++ != ':') {
      return;
    }
    uint32_t refresh = atoi(p);

    SetHdmiPreferredMode(w, h, refresh);
  }
}

HwcTestConfig& HwcTestState::GetTestConfig() {
  return mConfig;
}

HwcTestResult& HwcTestState::GetTestResult() {
  return mResult;
}

void HwcTestState::WaitForCompValToComplete() {
  if (mTestKernel) {
    mTestKernel->WaitForCompValToComplete();
  }
}

// Display property query
uint32_t HwcTestState::GetDisplayProperty(uint32_t displayIx,
                                          DisplayPropertyType prop) {
  if (mTestKernel) {
    return mTestKernel->GetDisplayProperty(displayIx, prop);
  } else {
    return 0;
  }
}

void HwcTestState::SetHdmiPreferredMode(uint32_t width, uint32_t height,
                                        uint32_t refresh) {
  if (mTestKernel) {
    mTestKernel->SetHdmiPreferredMode(width, height, refresh);
  }
}

bool HwcTestState::IsHotPluggableDisplayAvailable() {
  if (mTestKernel) {
    return mTestKernel->IsHotPluggableDisplayAvailable();
  } else {
    return false;
  }
}

// Direct test kernel to simulate hot plug
// returns true if hot plug was achieved
bool HwcTestState::SimulateHotPlug(bool connected, uint32_t displayTypes) {
  if (mTestKernel) {
    if (connected && mTestKernel->IsHotPluggableDisplayAvailable()) {
      // On hotplug, all existing protected sessions must be torn down.
      HWCLOGI("SimulateHotPlug: Encrypted sessions should be torn down soon.");
      mTestKernel->GetProtectionChecker().ExpectSelfTeardown();
    }

    bool hotPlugDone = mTestKernel->SimulateHotPlug(displayTypes, connected);

    if (!hotPlugDone) {
      // Direct call into HWC to pretend that a hotplug has been detected.
      // This to be used in DRM only, because under ADF we spoof the hotplug
      // event.
      HWCLOGD_COND(eLogHotPlug,
                   "Direct call into HWC to simulate hot%splug ENTER",
                   connected ? "" : "un");
      ALOG_ASSERT(mpHwcSimulateHotPlug);
      ++mHotPlugInProgress;
      mpHwcSimulateHotPlug(connected);
      --mHotPlugInProgress;

      if (mTestKernel) {
        mTestKernel->DoStall(connected ? Hwcval::eStallHotPlug
                                       : Hwcval::eStallHotUnplug);
      }

      HWCLOGD_COND(eLogHotPlug,
                   "Direct call into HWC to simulate hot%splug EXIT",
                   connected ? "" : "un");
    }
  } else {
    HWCLOGW("No shims, can't simulate hot plug");
  }

  if (displayTypes & eRemovable) {
    mNewDisplayConnectionState = connected;
  }

  return true;
}

bool HwcTestState::IsTotalDisplayFail() {
  if (mTestKernel) {
    return mTestKernel->IsTotalDisplayFail();
  } else {
    return false;
  }
}

// Expect a short period of inconsistency in protected content
void HwcTestState::NotifyProtectedContentChange() {
  if (mTestKernel) {
    mTestKernel->GetProtectionChecker().TimestampChange();
  }
}

int64_t HwcTestState::GetVBlankTime(uint32_t displayIx, bool& enabled) {
  if (mTestKernel) {
    HwcTestCrtc* crtc = mTestKernel->GetHwcTestCrtcByDisplayIx(displayIx);

    if (crtc) {
      int64_t t = crtc->GetVBlankTime(enabled);

      // If we haven't had a vblank, report when we started asking for them.
      return (t == 0) ? crtc->GetVBlankCaptureTime() : t;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

void HwcTestState::ProcessWork() {
  if (mTestKernel) {
    mTestKernel->ProcessWork();
  }
}

void HwcTestState::ReportPanelFitterStatistics(FILE* f) {
  if (mTestKernel) {
    for (uint32_t i = 0; i < HWCVAL_MAX_CRTCS; ++i) {
      HwcTestCrtc* crtc = mTestKernel->GetHwcTestCrtcByDisplayIx(i);

      if (crtc) {
        crtc->ReportPanelFitterStatistics(f);
      }
    }
  }
}

void HwcTestState::ReportFrameCounts(bool final) {
  if (mTestKernel) {
    if (final) {
      // Report if most recent ESD recovery event still not completed after
      // timeout period.
      mTestKernel->EsdRecoveryReport();

      mTestKernel->FinaliseTest();
    }

    mTestKernel->SendFrameCounts(final);
  }
}

void HwcTestState::ResetTestResults() {
  GetTestResult().Reset();
}

void HwcTestState::StopThreads() {
  if (mTestKernel) {
    mTestKernel->StopThreads();
  }
}

bool HwcTestState::IsFenceValid(int fence, uint32_t disp, uint32_t hwcFrame,
                                bool checkSignalled, bool checkUnsignalled) {
  return false;
}

bool HwcTestState::IsFenceSignalled(int fence, uint32_t disp,
                                    uint32_t hwcFrame) {
  return IsFenceValid(fence, disp, hwcFrame, true, false);
}

bool HwcTestState::IsFenceUnsignalled(int fence, uint32_t disp,
                                      uint32_t hwcFrame) {
  return IsFenceValid(fence, disp, hwcFrame, false, true);
}

void HwcTestState::TriggerOnSetCondition() {
  if (mOnSetConditionEnable) {
    HWCLOGD("HwcTestState::TriggerOnSetCondition");
    mOnSetCondition.signal();
  }
}

void HwcTestState::WaitOnSetCondition() {
  mOnSetConditionEnable = true;
  Hwcval::Mutex::Autolock lock(mOnSetMutex);
  mOnSetCondition.waitRelative(
      mOnSetMutex, 1000 * HWCVAL_MS_TO_NS);  // wait up to a second for OnSet
}

void HwcTestState::MarkEsdRecoveryStart(uint32_t connectorId) {
  if (mTestKernel) {
    mTestKernel->MarkEsdRecoveryStart(connectorId);
  }
}

void HwcTestState::SetWirelessCrtcParams(int32_t scaled_width,
                                         int32_t scaled_height,
                                         int32_t refresh) {
  mWirelessCrtcWidth = scaled_width;
  mWirelessCrtcHeight = scaled_height;
  mWirelessCrtcRefresh = refresh;
}

void HwcTestState::SetWirelessFrameCount(uint32_t frame_count) {
  mWirelessFrameCount = frame_count;
}

uint32_t HwcTestState::GetWirelessFrameCount() {
  return mWirelessFrameCount;
}

void HwcTestState::SetWirelessFrameTypeChangeCount(
    uint32_t frame_type_change_count) {
  mWirelessFrameTypeChangeCount = frame_type_change_count;
}

uint32_t HwcTestState::GetWirelessFrameTypeChangeCount() {
  return mWirelessFrameTypeChangeCount;
}

void HwcTestState::SetWirelessBufferInfoCount(uint32_t buffer_info_count) {
  mWirelessBufferInfoCount = buffer_info_count;
}

uint32_t HwcTestState::GetWirelessBufferInfoCount() {
  return mWirelessBufferInfoCount;
}

void HwcTestState::SetMaxSetResolutions(uint32_t max) {
  mMaxSetResolutions = max;
}

uint32_t HwcTestState::GetMaxSetResolutions(void) {
  return mMaxSetResolutions;
}

// Dynamically sets the dimensions for the Widi output buffer
void HwcTestState::SetWidiOutDimensions(uint32_t width, uint32_t height) {
  if (mTestKernel) {
    HwcTestCrtc* crtc =
        mTestKernel->GetHwcTestCrtcByDisplayIx(HWCVAL_VD_WIDI_DISPLAY_INDEX);

    if (crtc) {
      HWCLOGD("Setting Widi out dimensions to %d %d", width, height);

      crtc->SetOutDimensions(width, height);
    } else {
      HWCLOGD("Could not find Widi CRTC (id: %d)", HWCVAL_VD_WIDI_CRTC_ID);
    }
  } else {
    HWCLOGD("Invalid pointer to test kernel");
  }
}

void HwcTestState::SetMDSInfoProviderShim(
    Hwcval::MultiDisplayInfoProviderShim* shim) {
  mMDSInfoProviderShim = shim;
}

Hwcval::MultiDisplayInfoProviderShim* HwcTestState::GetMDSInfoProviderShim() {
  return mMDSInfoProviderShim;
}

uint32_t HwcTestState::GetHwcFrame(uint32_t displayIx) {
  if (mTestKernel) {
    return mTestKernel->GetHwcFrame(displayIx);
  } else {
    return 0;
  }
}

void HwcTestState::SetStall(Hwcval::StallType ix, const Hwcval::Stall& stall) {
  mStall[ix] = stall;
}

Hwcval::Stall& HwcTestState::GetStall(Hwcval::StallType ix) {
  return mStall[ix];
}

// Configure image dump
void HwcTestState::ConfigureImageDump(android::sp<Hwcval::Selector> selector,
                                      uint32_t maxDumpImages) {
  mFrameDumpSelector = selector;
  mMaxDumpImages = maxDumpImages;
}

void HwcTestState::ConfigureTgtImageDump(
    android::sp<Hwcval::Selector> selector) {
  mTgtFrameDumpSelector = selector;
}

uint32_t HwcTestState::TestImageDump(uint32_t hwcFrame) {
  if (mFrameDumpSelector.get() && mFrameDumpSelector->Test(hwcFrame)) {
    if (++mNumDumpImage <= mMaxDumpImages) {
      return mNumDumpImage;
    }
  }

  // Don't dump.
  return 0;
}

bool HwcTestState::TestTgtImageDump(uint32_t hwcFrame) {
  if (mTgtFrameDumpSelector.get()) {
    return mTgtFrameDumpSelector->Test(hwcFrame);
  }

  return false;
}

void HwcTestState::CheckRunningShims(uint32_t mask) {
  if ((mRunningShims & mask) != mask) {
    HWCERROR(eCheckSessionFail, "Shims running: 0x%x expected: 0x%x",
             mRunningShims, mask);
  }
}

void HwcTestState::LogToKmsg(const char* format, ...) {
  FILE* kmsg = fopen("/dev/kmsg", "w");
  va_list vl;

  va_start(vl, format);
  if (kmsg) {
    vfprintf(kmsg, format, vl);
    fclose(kmsg);
  }
  va_end(vl);
}

int HwcTestState::GetHwcOptionInt(const char* str) {
  if (mTestKernel) {
    return mTestKernel->GetHwcOptionInt(str);
  } else {
    return 0;
  }
}

const char* HwcTestState::GetHwcOptionStr(const char* str) {
  if (mTestKernel) {
    return mTestKernel->GetHwcOptionStr(str);
  } else {
    return 0;
  }
}

bool HwcTestState::IsAutoExtMode() {
  if (GetHwcOptionInt("extmodeauto")) {
    return true;
  } else {
    return IsOptionEnabled(eOptNoMds);
  }
}

// Notification from harness that a layer in the next OnSet will be transparent
void HwcTestState::SetFutureTransparentLayer(buffer_handle_t handle) {
  mFutureTransparentLayer = handle;
}

buffer_handle_t HwcTestState::GetFutureTransparentLayer() {
  return mFutureTransparentLayer;
}

void HwcTestState::SetVideoRate(uint32_t disp, float videoRate) {
  if (mTestKernel) {
    mTestKernel->SetVideoRate(disp, videoRate);
  }
}

const char* HwcTestState::DisplayTypeStr(uint32_t displayType) {
  switch (displayType) {
    case HwcTestState::eFixed:
      return "FIXED";

    case HwcTestState::eRemovable:
      return "REMOVABLE";

    case HwcTestState::eVirtual:
      return "VIRTUAL";

    default:
      return "MULTIPLE";
  }
}
