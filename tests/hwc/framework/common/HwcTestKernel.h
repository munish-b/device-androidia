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
#ifndef __HwcTestKernel_h__
#define __HwcTestKernel_h__

// NOTE: HwcTestDefs.h sets defines which are used in the HWC and DRM stack.
// -> to be included before any other HWC or DRM header file.
#include "HwcTestDefs.h"

#include "hardware/hwcomposer2.h"
#include <utils/SortedVector.h>
#include <utils/KeyedVector.h>
#include <utils/Mutex.h>
#include <ui/GraphicBuffer.h>

#include "HwcTestCrtc.h"
#include "DrmShimPlane.h"
#include "HwcvalLogDisplay.h"
#include "DrmShimCallbackBase.h"
#include "HwcTestUtil.h"

#include "HwcvalLayerListQueue.h"
#include "EventQueue.h"
#include "HwcvalWork.h"
#include "HwcvalSelector.h"
#include "HwcvalStatistics.h"

#include "HwcTestProtectionChecker.h"
#include "HwcTestCompValThread.h"

#include "HwcCrcReader.h"
#include "HwcvalLogParser.h"

class HwcTestState;
class DrmShimBuffer;
class HwcTestBufferObject;
class HwcServiceShim;

struct drm_mode_set_display;

#define EXPORT_API __attribute__((visibility("default")))
class EXPORT_API HwcTestKernel {
 public:
  struct BoKey {
    int fd;
    uint32_t h;
  };

  // Class of object
  enum ObjectClass { ePlane, eCrtc, eOther };

 protected:
  /// Pointer to test state
  HwcTestState* mState;

  /// Pointer to the CRC reader
  HwcCrcReader mCrcReader;

  /// Pointer to the HWCVAL log parser
  Hwcval::LogParser mLogParser;

  /// do not call real drm
  uint32_t mPassThrough;

  /// Mutex for internal state
  Hwcval::Mutex mMutex;

  // IVP file descriptor
  int mShimIvpFd;

  // Frame numbers at which the test started
  Hwcval::FrameNums mStartFN;

  // Per-display frame counts
  Hwcval::FrameNums mFN;

  // Number of SF displays in use
  // (actually 1+index of last active display)
  uint32_t mActiveDisplays;

  // gralloc buffer tracking by gralloc handle
  android::DefaultKeyedVector<buffer_handle_t, android::sp<DrmShimBuffer> >
      mBuffers;

  // buffer tracking by buffer "name" (this is actually the global handle of the
  // buffer)
  android::DefaultKeyedVector<int, android::sp<DrmShimBuffer> >
      mBuffersByGlobalId;

  // buffer tracking by buffer object handle (NOT same as gralloc handle)
  android::DefaultKeyedVector<BoKey, android::sp<HwcTestBufferObject> >
      mBosByBoHandle;

  // All planes, by plane id
  android::DefaultKeyedVector<uint32_t, DrmShimPlane*> mPlanes;

  // Layer list queue
  Hwcval::LayerListQueue mLLQ[HWCVAL_MAX_LOG_DISPLAYS];

  // Logical (SurfaceFlinger) displays
  Hwcval::LogDisplay mLogDisplays[HWCVAL_MAX_LOG_DISPLAYS];

  // Crtc for each physical display
  HwcTestCrtc* mCrtcByDisplayIx[HWCVAL_MAX_CRTCS];
  HwcTestCrtc* mPersistentCrtcByDisplayIx[HWCVAL_MAX_CRTCS];

  // Index of first video layer, per display
  int mVideoLayerIndex[HWCVAL_MAX_CRTCS];
  uint32_t mDisplayVideoCount[HWCVAL_MAX_CRTCS];
  buffer_handle_t mVideoHandle[HWCVAL_MAX_CRTCS];
  TriState mDisplayFullScreenVideo[HWCVAL_MAX_CRTCS];
  bool mActiveDisplay[HWCVAL_MAX_CRTCS];

  // A collection of logical z-orders per plane
  // The index is the z-order value passed into SetZorder
  android::DefaultKeyedVector<uint32_t, HwcTestCrtc::SeqVector*> mOrders;

  // All active video sessions (MDS mode only)
  android::DefaultKeyedVector<uint32_t, int32_t> mVideoSessions;
  // Current video rate (set by the harness) - non-MDS mode only
  float mVideoRate;

  // Input state, false means input timed out.
  bool mInputState;

  // Based on various information, do we think Extended Mode is called for?
  bool mRequireExtendedMode;
  HwcTestConfig::PanelModeType mRequireEMPanel;
  HwcTestConfig::PanelModeType mLastRequireEMPanel;
  uint32_t mFramesSinceEMPanelChange;

  // Flags set by the Widi shim:
  //   mWidiEnabled = widi (as opposed to a Virtual Display) is in operation
  //   mExtVideoModeDisabled = denotes whether extended mode has been disabled
  //   mBufferInfoRequire = whether a buffer info message is required
  //   mWidiFrameType = holds the expected frame type
  //   mNumSetResolutionsReceived = the number of received "Set Resolution"
  //   messages
  //   mWidiLastFrame = the number of the last frame received by the Widi shim
  enum WidiEnableState {
    eWidiDisabled = 0,
    eWidiEnabled,
    eWidiDisabling  // Widi disable commenced within the last frame
  };
  WidiEnableState mWidiState;
  bool mExtVideoModeDisabled;
  bool mBufferInfoRequired;
  uint32_t mNumSetResolutionsReceived;
  uint32_t mWidiLastFrame;

  // Composition Context
  DrmShimTransformVector mCompContributors;
  uint32_t mCompLayerIx;
  Hwcval::LayerList mCompSources;

  // List of composers whose activities we care about
  static const char* mComposerFilter;

  /// Gralloc
  struct gralloc_module_t* mGrallocModule;

  /// Target buffers for emulation of SF composition
  android::sp<android::GraphicBuffer> mTgtBuf[HWCVAL_MAX_CRTCS];

  /// Number of SF composition mismatches
  uint32_t mSfCompMismatchCount;

  /// Composition Validator
  android::sp<HwcTestCompValThread> mCompVal;

  // A queue for Gem Open/Close/Flink events.
  // These must be queued to break a deadlock between gralloc and ourselves.
  Hwcval::Work::Queue mWorkQueue;

  // For checking if protected content has valid session/instance
  HwcTestProtectionChecker mProtChecker;

  // Time last onPrepare started
  int64_t mLastOnPrepareTime;

#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE
  // Shim of HWC IService
  android::sp<HwcServiceShim> mHwcServiceShim;
#endif  // HWCVAL_BUILD_SHIM_HWCSERVICE

  uint32_t mPrefHdmiWidth;
  uint32_t mPrefHdmiHeight;
  uint32_t mPrefHdmiRefresh;

  // DDR mode
  uint64_t mDDRMode;
  uint64_t mDDRModeLastFrame;
  bool mForceLowDDRMode;
  bool mLastForceLowDDRMode;
  uint32_t mChangingDDRMode;

  typedef android::KeyedVector<android::String8, android::String8> HwcOptionVec;
  HwcOptionVec mHwcOptions;

  // Wireless Docking Constructs
  bool mWirelessDockingIsActive = false;
  int mWirelessDockingFramesToWait = 0;

  // Rotation snapshot detection
  // HWC does strange things with snapshots so we need to know this is going on
  // in the validation
  // so we can ignore the errors we would be incliend to generate.
  //
  // Key: Handles of snapshot buffers
  // Value: frame number at which the snapshot expires.
  android::KeyedVector<buffer_handle_t, uint32_t> mSnapshots;

  // Expiry of all snapshots. Tells us we will stay in Extended Mode.
  uint32_t mRotationStartFrame;
  uint32_t mRotationEndFrame;

  // Statistics
  Hwcval::Statistics::Histogram mCompTargets;
  Hwcval::Statistics::Aggregate<uint32_t> mTotalBuffers;
  Hwcval::Statistics::Counter mSfCompositionCount;
  Hwcval::Statistics::Counter mIvpCompositionCount;
  Hwcval::Statistics::Counter mPartitionedCompositionCount;
  Hwcval::Statistics::Counter mWritebackCompositionCount;

  Hwcval::Statistics::Aggregate<double> mPCScaleStat;
  Hwcval::Statistics::Aggregate<double> mIvpScaleStat;
  Hwcval::Statistics::Aggregate<double> mSfScaleStat;
  Hwcval::Statistics::Counter mSnapshotsRestored;

 public:
  //-----------------------------------------------------------------------------
  // Constructor & Destructor
  HwcTestKernel();
  virtual ~HwcTestKernel();

  // Public interface used by the test

  EXPORT_API bool passThrough();

#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE
  // Shim of HWC IService
  EXPORT_API void SetHwcServiceShim(android::sp<HwcServiceShim> serviceShim);
#endif  // HWCVAL_BUILD_SHIM_HWCSERVICE

  // Initialise video flags at start of onSet processing
  void VideoInit(uint32_t displayIx);

  /// Call NextFrame on all buffers
  void IterateAllBuffers();

  /// Implements checks for Widi buffers
  EXPORT_API android::sp<DrmShimBuffer> lookupDrmShimBuffer(
      buffer_handle_t handle);
  EXPORT_API void checkWidiBuffer(HwcTestCrtc* crtc, Hwcval::LayerList* ll,
                                  buffer_handle_t handle);
  EXPORT_API void checkWidiBuffer(buffer_handle_t handle);

  EXPORT_API uint32_t GetWidiLastFrame() {
    return mWidiLastFrame;
  }
  EXPORT_API void setBufferInfoRequired(bool bufferInfoRequired) {
    mBufferInfoRequired = bufferInfoRequired;
  }
  EXPORT_API void IncNumSetResolutions() {
    ++mNumSetResolutionsReceived;
  }

  EXPORT_API bool IsWidiEnabled();
  EXPORT_API bool IsWidiDisabled();

  EXPORT_API void setExtVideoModeDisable(bool disableExtVideoMode) {
    mExtVideoModeDisabled = disableExtVideoMode;
  }

  /// Do we meet the conditions for widi video mode validation?
  EXPORT_API bool IsWidiVideoMode(Hwcval::LayerList::VideoFlags vf,
                                  uint32_t hwcFrame);

  // Complete Widi state transition to disabled (if started)
  EXPORT_API void CompleteWidiDisable();

  // Provide harness test with a way to wait until composition validation
  // finishes
  EXPORT_API void WaitForCompValToComplete();

  // HWC's page flip handler has run and completed
  void NotifyPageFlipHandlerExit(HwcTestCrtc* crtc,
                                 int firstUnsignalledRetireFence);

  // Throw away the next frame we were going to validate
  void SkipFrameValidation(HwcTestCrtc* crtc);

  // Read anything outstanding in the LLQ to work out how many frames were
  // dropped at the end of the test
  void FinaliseTest();

  /// Intercepted Ioctl calls
  /// Create a BO handle for a "named" buffer
  EXPORT_API void CheckIoctlGemOpen(int fd, struct drm_gem_open* gemOpen);

  /// Close a BO handle
  EXPORT_API void CheckIoctlGemClose(int fd, struct drm_gem_close* gemClose);

  /// Create a global name for a buffer
  EXPORT_API void CheckIoctlGemFlink(int fd, struct drm_gem_flink* flink);

  /// Open a buffer object handle
  void CheckIoctlGemCreate(int fd, struct drm_i915_gem_create* gemCreate);

  /// Wait for a buffer object
  void CheckIoctlGemWait(int fd, struct drm_i915_gem_wait* gemWait, int status,
                         int64_t delayNs);

  /// Associate a DMA buffer with a buffer object handle.
  void CheckIoctlPrime(int fd, struct drm_prime_handle* prime);

  /// Generate warning if display disabled
  EXPORT_API void ReportIfDisplayDisabled();

  /// Copy frame counts to HwcTestResult for transmission back to the test
  EXPORT_API void SendFrameCounts(bool clear);

  /// MDS callbacks, for extended mode checking
  EXPORT_API void UpdateVideoState(int64_t sessionId, bool isPrepared);
  EXPORT_API void UpdateVideoStateLegacy(int sessionId, uint32_t state);
  EXPORT_API void UpdateVideoFPS(int64_t sessionId, int32_t fps);
  EXPORT_API void UpdateInputState(bool state);

  // Check if any encrypted buffers with invalidated session/instance id
  // are currently displayed.
  void CheckInvalidSessionsDisplayed();

  // Do we have any hotpluggable display connected?
  virtual EXPORT_API bool IsHotPluggableDisplayAvailable() = 0;

  // Simulate hotplug on any suitable display
  virtual EXPORT_API bool SimulateHotPlug(uint32_t displayTypes,
                                          bool connected) = 0;

  // Access to extended mode state
  bool IsExtendedModeStable();
  bool IsExtendedModeRequired();
  bool IsEMPanelOffRequired();
  bool IsEMPanelOffAllowed();
  const char* EMPanelStr();

  // Set number of active displays
  void SetActiveDisplays(uint32_t activeDisplays);

  // Access to mutex
  Hwcval::Mutex& GetMutex();

  void StopThreads();

  /// Processing functions for Gem events
  void DoGem(const Hwcval::Work::GemOpenItem& item);
  void DoGem(const Hwcval::Work::GemCloseItem& item);
  void DoGem(const Hwcval::Work::GemFlinkItem& item);
  void DoGem(const Hwcval::Work::GemCreateItem& item);
  void DoGem(const Hwcval::Work::GemWaitItem& item);

  // Processing function for Prime call (associate bo handle with DMA buffer
  // handle)
  // Implementation was in Adf
  virtual void DoPrime(const Hwcval::Work::PrimeItem& item);

  // Processing function for notification of buffer destruction
  void DoBufferFree(const Hwcval::Work::BufferFreeItem& item);

  HwcTestCrtc* GetHwcTestCrtcByDisplayIx(uint32_t displayIx,
                                         bool persistentCopy = false);

  // Get reference to Protection checker
  HwcTestProtectionChecker& GetProtectionChecker();

  // Get reference to CRC Reader
  HwcCrcReaderInterface& GetCrcReader();

  // Get logical display
  Hwcval::LogDisplay& GetLogDisplay(uint32_t displayIx);

  // Get current frame number of last OnSet handled
  uint32_t GetHwcFrame(uint32_t displayIx);
  const Hwcval::FrameNums& GetFrameNums() const;

  // Returns the parser object
  virtual Hwcval::LogChecker* GetParser() = 0;

  // Translate CRTC Id to display index
  uint32_t CrtcIdToDisplayIx(uint32_t crtcId);

  /// Determine from MDS inputs etc whether we should be in extended mode
  void SetExtendedModeExpectation(bool singleFullScreenVideo,
                                  bool haveSecondDisplay, uint32_t hwcFrame);

  // Display property query
  virtual uint32_t GetDisplayProperty(
      uint32_t displayIx, HwcTestState::DisplayPropertyType prop) = 0;

  /// Override HDMI preferred mode
  void SetHdmiPreferredMode(uint32_t width = 0, uint32_t height = 0,
                            uint32_t refresh = 0);

  // Wireless Docking constructs
  void SetWirelessDockingMode(bool mode);
  bool GetWirelessDockingMode(void);

  /// ESD Recovery
  virtual void MarkEsdRecoveryStart(uint32_t connectorId);
  void EsdRecoveryReport();

  /// Is display in a bad state?
  bool IsTotalDisplayFail();

  // Video analysis - do we have full screen video, etc
  void SetActiveDisplay(uint32_t displayIx, bool active);
  void DetermineFullScreenVideo(uint32_t displayIx, uint32_t i,
                                Hwcval::ValLayer& vl, const char* notes);
  Hwcval::LayerList::VideoFlags AnalyzeVideo();

  /// What is the video rate as indicated by MDS?
  uint32_t GetMDSVideoRate();

  /// Harness entry point to set expected video rate on each display
  void SetVideoRate(uint32_t disp, float videoRate);

  // Configure/Use stalls
  void SetStall(Hwcval::StallType ix, const Hwcval::Stall& stall);
  void DoStall(Hwcval::StallType ix, Hwcval::Mutex* mtx = 0);
  // Hwc options
  const char* GetHwcOptionStr(const char* optionName);
  int GetHwcOptionInt(const char* optionName);

  // Parse option set for display config
  // (We already know this is the DmConfig option value so no need to return
  // true/false)
  void ParseDmConfig(const char* str);
  void MapLogDisplays(android::Vector<Hwcval::LogDisplayMapping> mappings);

  // Work Queue Accessor
  Hwcval::Work::Queue& GetWorkQueue();

  // Harness can call this regularly to avoid the work queue getting too big
  // when
  // buffers are being created
  EXPORT_API void ProcessWork();
  EXPORT_API void ProcessWorkLocked();

  // Hwc Options
  void SetHwcOption(android::String8& optionName, android::String8& value);

  /// Memory optimization mode
  void ValidateOptimizationMode(Hwcval::LayerList* disp);
  virtual bool IsDDRFreqSupported() = 0;

  /// Set maximum scale factor (X or Y) we will allow through to iVP
  void SetIvpScaleRange(float minScale, float maxScale);

  /// Set snapshot handle and lifetime
  void SetSnapshot(buffer_handle_t snapshotHandle, uint32_t keepCount);
  bool IsSnapshot(buffer_handle_t handle, uint32_t hwcFrame);
  bool IsRotationInProgress(uint32_t hwcFrame);

  // Set frame number per display
  void AdvanceFrame(uint32_t d, uint32_t hwcFrame);
  void AdvanceFrame(uint32_t d);

  /// Note in the buffer list that HWC has been asked to draw one
  android::sp<DrmShimBuffer> RecordBufferState(
      buffer_handle_t handle, Hwcval::BufferSourceType bufferSource,
      char* notes);

  /// Validate layer display frame is inside Framebuffer target
  void ValidateHwcDisplayFrame(const hwc_rect_t& layerDf,
                               const hwc_rect_t& fbtDf, uint32_t displayIx,
                               uint32_t layerIx);

  /// Get the layer list queue for the stated display
  Hwcval::LayerListQueue& GetLLQ(uint32_t displayIx);

  /// Get the composition validator
  android::sp<HwcTestCompValThread> GetCompVal();

  /// Statistics
  void AddSfScaleStat(double scale);
  void IncSfCompositionCount();

 protected:
  /// Remove Buffer Object
  void RemoveBo(BoKey k, const char* str);

  /// Create a tracking object for a buffer object
  virtual HwcTestBufferObject* CreateBufferObject(int fd,
                                                  uint32_t boHandle) = 0;

  /// Find or create and index a tracking object for a bo
  virtual android::sp<HwcTestBufferObject> GetBufferObject(
      uint32_t boHandle) = 0;

  /// Move device-specific ids from old to new buffer
  virtual void MoveDsIds(android::sp<DrmShimBuffer> existingBuf,
                         android::sp<DrmShimBuffer> buf) = 0;

  // Internal validation
  void ValidateBo(android::sp<DrmShimBuffer> buf, const char* str = "");
  void ValidateBo(android::sp<HwcTestBufferObject> bo,
                  android::sp<DrmShimBuffer> buf, const char* str = "");

  /// Make an educated guess as to whether this is an empty buffer used by HWC
  /// to replace prohibited content
  bool BelievedEmpty(buffer_handle_t handle);
  bool BelievedEmpty(uint32_t width, uint32_t height);

 private:
  /// Look for excessive scale factors
  bool CheckIvpScaling(iVP_layer_t* ivpLayer);

  /// Check consistency of one set of coordinates input to IVP with SF layer
  android::sp<DrmShimBuffer> IvpCoordinateCheck(
      DrmShimTransformVector& contributors,
      android::Vector<hwcval_layer_t>& sfLayers, uint32_t layerIx,
      iVP_layer_t* ivpLayer, buffer_handle_t outHandle, bool& err,
      const char* description, int param);

  /// Get DrmShimBuffer for the handle, and check encryption state has not
  /// changed
  android::sp<DrmShimBuffer> UpdateIvpBufferState(buffer_handle_t handle);

  /// Add next sequence to one of the supported z-orders
  void AddZOrder(uint32_t order, uint32_t seq, uint32_t planeOffset);

  /// Map a global ID to a buffer, and remove any previous mapping
  void MapGlobalId(int id, android::sp<DrmShimBuffer> buf);
};

inline void HwcTestKernel::AdvanceFrame(uint32_t d) {
  ++mFN[d];
  mLastOnPrepareTime = systemTime(SYSTEM_TIME_MONOTONIC);
}

inline void HwcTestKernel::AdvanceFrame(uint32_t d, uint32_t hwcFrame) {
  mFN[d] = hwcFrame;
  mLastOnPrepareTime = systemTime(SYSTEM_TIME_MONOTONIC);
}

inline bool HwcTestKernel::passThrough() {
  return mPassThrough;
}

inline void HwcTestKernel::UpdateInputState(bool state) {
  mInputState = state;
}

inline bool HwcTestKernel::IsExtendedModeStable() {
  return (mFramesSinceEMPanelChange > 6);
}

inline bool HwcTestKernel::IsExtendedModeRequired() {
  return mRequireExtendedMode;
}

inline bool HwcTestKernel::IsEMPanelOffRequired() {
  return mRequireEMPanel == HwcTestConfig::eOff;
}

inline bool HwcTestKernel::IsEMPanelOffAllowed() {
  return mRequireEMPanel != HwcTestConfig::eOn;
}

inline const char* HwcTestKernel::EMPanelStr() {
  return HwcTestConfig::Str(mRequireEMPanel);
}

inline Hwcval::Mutex& HwcTestKernel::GetMutex() {
  return mMutex;
}

inline HwcTestProtectionChecker& HwcTestKernel::GetProtectionChecker() {
  return mProtChecker;
}

inline HwcCrcReaderInterface& HwcTestKernel::GetCrcReader() {
  return mCrcReader;
}

inline Hwcval::LogDisplay& HwcTestKernel::GetLogDisplay(uint32_t displayIx) {
  return mLogDisplays[displayIx];
}

inline uint32_t HwcTestKernel::GetHwcFrame(uint32_t displayIx) {
  return mFN[displayIx];
}

inline const Hwcval::FrameNums& HwcTestKernel::GetFrameNums() const {
  return mFN;
}

inline Hwcval::Work::Queue& HwcTestKernel::GetWorkQueue() {
  return mWorkQueue;
}

inline Hwcval::LayerListQueue& HwcTestKernel::GetLLQ(uint32_t displayIx) {
  return mLLQ[displayIx];
}

#ifndef HWCVAL_INTERNAL_BO_VALIDATION
inline void HwcTestKernel::ValidateBo(android::sp<DrmShimBuffer> buf,
                                      const char* str) {
  HWCVAL_UNUSED(buf);
  HWCVAL_UNUSED(str);
}

inline void HwcTestKernel::ValidateBo(android::sp<HwcTestBufferObject> bo,
                                      android::sp<DrmShimBuffer> buf,
                                      const char* str) {
  HWCVAL_UNUSED(bo);
  HWCVAL_UNUSED(buf);
  HWCVAL_UNUSED(str);
}
#endif

inline void HwcTestKernel::SetHdmiPreferredMode(uint32_t width, uint32_t height,
                                                uint32_t refresh) {
  mPrefHdmiWidth = width;
  mPrefHdmiHeight = height;
  mPrefHdmiRefresh = refresh;
}

inline bool HwcTestKernel::IsWidiEnabled() {
  return (mWidiState == eWidiEnabled);
}

// Widi disabled state excludes the transitional one after Widi disconnect
// commences
inline bool HwcTestKernel::IsWidiDisabled() {
  return (mWidiState == eWidiDisabled);
}

inline void HwcTestKernel::SetWirelessDockingMode(bool mode) {
  mWirelessDockingFramesToWait = 2;
  mWirelessDockingIsActive = mode;
}

inline bool HwcTestKernel::GetWirelessDockingMode(void) {
  return mWirelessDockingIsActive;
}

/// Get the composition validator
inline android::sp<HwcTestCompValThread> HwcTestKernel::GetCompVal() {
  return mCompVal;
}

inline void HwcTestKernel::AddSfScaleStat(double scale) {
  mSfScaleStat.Add(scale);
}

inline void HwcTestKernel::IncSfCompositionCount() {
  ++mSfCompositionCount;
}

inline void HwcTestKernel::SetActiveDisplays(uint32_t activeDisplays) {
  mActiveDisplays = activeDisplays;
}

inline void HwcTestKernel::SetActiveDisplay(uint32_t displayIx, bool active) {
  mActiveDisplay[displayIx] = active;
}

// Nonmember operators
inline bool operator<(const HwcTestKernel::BoKey& lhs,
                      const HwcTestKernel::BoKey& rhs) {
  union BoKeyCmp {
    HwcTestKernel::BoKey k;
    uint64_t i;
  };

  BoKeyCmp lhscmp;
  BoKeyCmp rhscmp;

  lhscmp.k = lhs;
  rhscmp.k = rhs;

  return (lhscmp.i < rhscmp.i);
}

#endif  // __HwcTestKernel_h__