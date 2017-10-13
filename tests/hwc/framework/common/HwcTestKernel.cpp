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

#include <unistd.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <math.h>

#include "HwcTestDefs.h"
#include "HwcTestKernel.h"
#include "HwcTestUtil.h"
#include "DrmShimBuffer.h"
#include "BufferObject.h"
#include "HwcvalGeom.h"
#include "HwcTestState.h"
#include "HwcTestConfig.h"
#include "HwcTestReferenceComposer.h"
#include "HwcTestDebug.h"
#include "HwcvalThreadTable.h"
#include "HwcvalHwc1Content.h"
#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE
#include "HwcServiceShim.h"
#endif

#include "drm_fourcc.h"
#include "i915_drm.h"

#ifdef HWCVAL_TARGET_HAS_MULTIPLE_DISPLAY
#include "MultiDisplayType.h"
#endif

#undef LOG_TAG
#define LOG_TAG "DRM_SHIM"

using namespace Hwcval;

DrmShimCallbackBase* drmShimCallback = 0;

static HwcTestReferenceComposer sRefCmp;

#ifdef HWCVAL_INTERNAL_BO_VALIDATION
static void BoValidationAssert() {
  if (strcmp(S(HWCVAL_INTERNAL_BO_VALIDATION), "ASSERT") == 0) {
    sleep(1);
    ALOG_ASSERT(0);
  }
}
#endif

/// Constructor
HwcTestKernel::HwcTestKernel()
    : mState(HwcTestState::getInstance()),
      mCrcReader(this, mState),
      mLogParser(this),
      mPassThrough(1),
      mPlanes((DrmShimPlane*)0),
      mOrders((HwcTestCrtc::SeqVector*)0),
      mVideoRate(0),
      mInputState(true),  // input not timed out
      mRequireExtendedMode(false),
      mRequireEMPanel(HwcTestConfig::eOn),
      mLastRequireEMPanel(HwcTestConfig::eOn),
      mFramesSinceEMPanelChange(HWCVAL_EXTENDED_MODE_CHANGE_WINDOW),
      mExtVideoModeDisabled(false),
      mBufferInfoRequired(false),
      mNumSetResolutionsReceived(0),
      mWidiLastFrame(0),
      mSfCompMismatchCount(0),
      mLastOnPrepareTime(0),
#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE
      mVideoControl(0),
#endif
      mPrefHdmiWidth(0),
      mPrefHdmiHeight(0),
      mPrefHdmiRefresh(0),
      mDDRMode(0),
      mDDRModeLastFrame(0),
      mForceLowDDRMode(0),
      mChangingDDRMode(0),
      mRotationStartFrame(0),
      mRotationEndFrame(0),

      // Statistics
      mCompTargets("composition_targets"),
      mTotalBuffers("total_buffers"),
      mSfCompositionCount("sf_compositions"),
      mPartitionedCompositionCount("partitioned_compositions"),
      mWritebackCompositionCount("writeback_compositions"),
      mPCScaleStat("partitioned_composer_scale", "%f"),
      mSfScaleStat("sf_scale_stat", "%f"),
      mSnapshotsRestored("snapshots_restored") {
  HWCLOGI("Creating HwcTestKernel");

  memset(mCrtcByDisplayIx, 0, sizeof(mCrtcByDisplayIx));
  memset(mPersistentCrtcByDisplayIx, 0, sizeof(mPersistentCrtcByDisplayIx));

  // Create the Crtc and plane objects for the Widi display
  HWCLOGD("Initialising CRTC for the Widi display");

  HwcTestCrtc* virtCrtc = new HwcTestCrtc(HWCVAL_VD_CRTC_ID, 0, 0, 0, 0);
  HwcTestCrtc* disp0Crtc = new HwcTestCrtc(0, 0, 0, 0, 0);
  DrmShimPlane* mainPlane = new DrmShimPlane(HWCVAL_VD_CRTC_ID, virtCrtc);
  mainPlane->SetPlaneIndex(0);
  virtCrtc->AddPlane(mainPlane);
  disp0Crtc->AddPlane(mainPlane);
  mCrtcByDisplayIx[eDisplayIxVirtual] = virtCrtc;
  mCrtcByDisplayIx[eDisplayIxFixed] = disp0Crtc;
  mPersistentCrtcByDisplayIx[eDisplayIxVirtual] = virtCrtc;
  mPersistentCrtcByDisplayIx[eDisplayIxFixed] = disp0Crtc;
  virtCrtc->SetDisplayIx(eDisplayIxVirtual);
  disp0Crtc->SetDisplayIx(eDisplayIxFixed);
  virtCrtc->SetDimensions(mState->GetWirelessCrtcWidth(),
                          mState->GetWirelessCrtcHeight(), 0,
                          mState->GetWirelessCrtcRefresh());
  // Start composition validation thread
  mCompVal = new HwcTestCompValThread();

// Setup z-order lookup for Valleyview
// TODO: Support other platforms
#ifdef PASASBCA
  AddZOrder(PASASBCA, 0, 0);
  AddZOrder(PASASBCA, 1, 1);
  AddZOrder(PASASBCA, 2, 2);

  AddZOrder(PASBSACA, 0, 0);
  AddZOrder(PASBSACA, 1, 2);
  AddZOrder(PASBSACA, 2, 1);

  AddZOrder(SBPASACA, 0, 2);
  AddZOrder(SBPASACA, 1, 0);
  AddZOrder(SBPASACA, 2, 1);

  AddZOrder(SBSAPACA, 0, 2);
  AddZOrder(SBSAPACA, 1, 1);
  AddZOrder(SBSAPACA, 2, 0);

  AddZOrder(SAPASBCA, 0, 1);
  AddZOrder(SAPASBCA, 1, 0);
  AddZOrder(SAPASBCA, 2, 2);

  AddZOrder(SASBPACA, 0, 1);
  AddZOrder(SASBPACA, 1, 2);
  AddZOrder(SASBPACA, 2, 0);
#endif

  // Get gralloc module
  hw_module_t const* module;
  int err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
  ALOG_ASSERT(!err);

  mGrallocModule = (struct gralloc_module_t*)module;

  for (uint32_t i = 0; i < HWCVAL_MAX_LOG_DISPLAYS; ++i) {
    mLogDisplays[i].SetDisplayIx(i);
    mLLQ[i].SetId(i);
  }

  for (uint32_t i = 0; i < HWCVAL_MAX_CRTCS; ++i) {
    mFN[i] = HWCVAL_UNDEFINED_FRAME_NUMBER;
  }

  mStartFN = mFN;
}

/// Destructor
HwcTestKernel::~HwcTestKernel() {
  HWCLOGI("Destroying HwcTestKernel");
  mWorkQueue.Closedown();

  // Kill the composition validation thread
  mCompVal->KillThread();

  for (uint32_t i = 0; i < mOrders.size(); ++i) {
    delete mOrders.valueAt(i);
  }

  for (uint32_t i = 0; i < HWCVAL_MAX_CRTCS; ++i) {
    if (mCrtcByDisplayIx[i]) {
      delete mCrtcByDisplayIx[i];
    }
  }
}

#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE
// Set Shim of HWC IService
EXPORT_API void HwcTestKernel::SetHwcServiceShim(
    android::sp<HwcServiceShim> serviceShim) {
  mHwcServiceShim = serviceShim;
}
#endif  // HWCVAL_BUILD_SHIM_HWCSERVICE

void HwcTestKernel::IterateAllBuffers() {
  // Update state on our internal record of gralloc buffers
  // We only need to do this for ones seen by entry to OnSet
  HWCLOGV("IterateAllBuffers: mBuffers.size()=%d DrmShimBuffer::mCount=%d",
          mBuffers.size(), DrmShimBuffer::mCount);

  if ((mBuffers.size() > CURRENT_BUFFER_COUNT_WARNING_LEVEL) ||
      (DrmShimBuffer::mCount > TOTAL_BUFFER_COUNT_WARNING_LEVEL)) {
    HWCERROR(eCheckObjectLeak,
             "Too many buffer records: mBuffers.size()=%d, Total active buffer "
             "records=%d",
             mBuffers.size(), DrmShimBuffer::mCount);
  }

  // Count how many composition buffers we have
  uint32_t compTargets = 0;

  for (uint32_t i = 0; i < mBuffers.size(); ++i) {
    android::sp<DrmShimBuffer> buf = mBuffers.valueAt(i);

    if (buf->IsCompositionTarget()) {
      ++compTargets;
    }
  }

  // Add to the statistics
  mCompTargets.Add(compTargets);
  mTotalBuffers.Add(mBuffers.size());
}

// Initialise video flags at start of onSet processing
void HwcTestKernel::VideoInit(uint32_t displayIx) {
  mVideoLayerIndex[displayIx] = -1;
  mDisplayVideoCount[displayIx] = 0;
  mDisplayFullScreenVideo[displayIx] = eUndefined;

  if (displayIx == 0) {
    for (uint32_t i = 0; i < HWCVAL_MAX_CRTCS; ++i) {
      mVideoHandle[i] = 0;
    }
  }
}

// For a particular display, work out whether it contains full screen video.
void HwcTestKernel::DetermineFullScreenVideo(uint32_t displayIx, uint32_t i,
                                             Hwcval::ValLayer& vl,
                                             const char* notes) {
  char strbuf[HWCVAL_DEFAULT_STRLEN];
  android::sp<DrmShimBuffer> buf = vl.GetBuf();

  if (buf.get() == 0) {
    return;
  }

  HwcTestCrtc* crtc = GetHwcTestCrtcByDisplayIx(displayIx);

  // Look for a static layer in front of the video
  int format = buf->GetFormat();
  if (mVideoLayerIndex[displayIx] < 0) {
    if (buf->IsVideoFormat()) {
      mVideoLayerIndex[displayIx] = i;
    }
  } else if (buf->GetAppearanceCount() > 25) {
    // It's a static layer in front of video.
    // Is it RGBA/BGRA?
    if ((buf->GetFormat() == HAL_PIXEL_FORMAT_RGBA_8888) ||
        (buf->GetFormat() == HAL_PIXEL_FORMAT_BGRA_8888)) {
      // Take a copy of the buffer, unless we've done so already
      mCompVal->TakeCopy(buf);
    }
  }

  // Try to work out
  // if all screens are showing the same video, and if it's full screen in one
  // place.
  //
  // NOTE: The famous 1-page spec states that full screen video mode is only
  // engaged
  // if the video is at the back of the stack.
  // This is now incorrect, as Chrome may add layers behind the video.
  if (buf->IsVideoFormat()) {
    ++mDisplayVideoCount[displayIx];
    mVideoHandle[displayIx] = buf->GetHandle();

    if (crtc) {
      const hwc_rect_t df = vl.GetDisplayFrame();
      uint32_t width = df.right - df.left;
      uint32_t height = df.bottom - df.top;
      float areaRatio =
          float(width * height) / float(crtc->GetWidth() * crtc->GetHeight());

      HWCLOGD_COND(eLogVideo,
                   "Display %d layer %d (bottom) (l,t,r,b) = (%d, %d, %d, %d) "
                   "area ratio %f",
                   displayIx, i, df.left, df.top, df.right, df.bottom,
                   (double)areaRatio);

      if ((abs(int(width) - int(crtc->GetWidth())) <= 1) ||
          (abs(int(height) - int(crtc->GetHeight())) <= 1) ||
          (areaRatio >= 0.9)) {
        HWCLOGD_COND(eLogVideo, "Display %d full screen video", displayIx);
        mDisplayFullScreenVideo[displayIx] =
            eTrue;  // This condition only needs to be true on one screen
      } else {
        mDisplayFullScreenVideo[displayIx] = eFalse;
      }

      HWCLOGV_COND(eLogVideo, "D%d.%u Fmt %s is video", displayIx, i,
                   buf->StrBufFormat());
    }
  }

  HWCLOGI_COND(eLogBuffer, "D%d.%u Fmt 0x%x %s Appearances %d %s FS:%s %s",
               displayIx, i, format, (buf.get() ? buf->IdStr(strbuf) : "0"),
               buf->GetAppearanceCount(), buf->IsVideoFormat() ? "VIDEO " : "",
               TriStateStr(mDisplayFullScreenVideo[displayIx]), notes);

  if (vl.GetCompositionType() != Hwcval::CompositionType::TGT) {
    uint32_t ctr = mState->TestImageDump(mFN[displayIx]);

    if (ctr > 0) {
      HWCNativeHandle handle = buf->GetHandle();
      HwcTestDumpGrallocBufferToDisk("main", ctr, handle, DUMP_BUFFER_TO_TGA);
      HwcTestDumpAuxBufferToDisk("aux", ctr, handle);
    }
  }
}

// Work out combined video state flags, by looking at the current state of all
// displays
Hwcval::LayerList::VideoFlags HwcTestKernel::AnalyzeVideo() {
  Hwcval::LayerList::VideoFlags videoFlags;

  // Work out if we one have video on D0; no other video showing;
  // and optionally the video showing on other displays too
  bool allScreenVideo = true;
  TriState fullScreenVideo = eUndefined;

  if (mDisplayVideoCount[0] != 1) {
    allScreenVideo = false;
  } else {
    for (uint32_t displayIx = 1; displayIx < HWCVAL_MAX_CRTCS; ++displayIx) {
      if (mDisplayVideoCount[displayIx] > 1) {
        allScreenVideo = false;
      } else if (mDisplayVideoCount[displayIx] == 1) {
        if (mVideoHandle[displayIx] != mVideoHandle[0]) {
          allScreenVideo = false;
        }
      } else if (mActiveDisplay[displayIx]) {
        allScreenVideo = false;
      }

      fullScreenVideo = (fullScreenVideo || mDisplayFullScreenVideo[displayIx]);
    }
  }

  bool singleFullScreenVideo = allScreenVideo && (fullScreenVideo == eTrue);
  bool partialScreenVideo = allScreenVideo && (fullScreenVideo == eFalse);

  HWCLOGD_COND(eLogVideo,
               "Frame:%d:%d:%d mSingleFullScreenVideo = (allScreenVideo=%d) && "
               "(fullScreenVideo=%s) = %d. PartialScreenVideo=%d",
               mFN[0], mFN[1], mFN[2], allScreenVideo,
               TriStateStr(fullScreenVideo), singleFullScreenVideo,
               partialScreenVideo);

  // Save the video flags in every display entered in the LLQ
  videoFlags.mSingleFullScreenVideo = singleFullScreenVideo;
  videoFlags.mFullScreenVideo = fullScreenVideo;
  videoFlags.mPartScreenVideo = partialScreenVideo;

  return videoFlags;
}

// Determine how a layer's display frame intersects with the screen: Is it fully
// visible, partially off screen, or
// completely off screen.
void HwcTestKernel::ValidateHwcDisplayFrame(const hwc_rect_t& layerDf,
                                            const hwc_rect_t& fbtDf,
                                            uint32_t displayIx,
                                            uint32_t layerIx) {
  Hwcval::OverlapType overlap = Hwcval::AnalyseOverlap(layerDf, fbtDf);
  HwcTestCheckType check;

  switch (overlap) {
    case Hwcval::eEnclosed:
      // layer is fully on screen. No error or warning.
      return;

    case Hwcval::eOverlapping:
      check = eCheckLayerPartlyOnScreen;
      break;

    case Hwcval::eOutside:
      check = eCheckLayerOnScreen;
      break;

    default:
      ALOG_ASSERT(0);
  }

  HWCERROR(check,
           "Display %d Layer %d Displayframe (%d, %d, %d, %d) screen (%d, %d, "
           "%d, %d)",
           displayIx, layerIx, layerDf.left, layerDf.top, layerDf.right,
           layerDf.bottom, fbtDf.left, fbtDf.top, fbtDf.right, fbtDf.bottom);
}

// Function to lookup a DrmShimBuffer from the gralloc handle.
android::sp<DrmShimBuffer> HwcTestKernel::lookupDrmShimBuffer(
    HWCNativeHandle handle) {
  ssize_t ix = mBuffers.indexOfKey(handle);
  if (ix < 0) {
    HWCLOGD_COND(eLogBuffer, "Could not find DrmShimBuffer for handle " PRIi64,
                 handle);
  }

  return (ix >= 0) ? mBuffers[ix] : NULL;
}

void HwcTestKernel::NotifyPageFlipHandlerExit(HwcTestCrtc* crtc,
                                              int firstUnsignalledRetireFence) {
  PushThreadState ts("NotifyPageFlipHandlerExit");
  crtc->PageFlipsSinceDPMS();
  HWCVAL_UNUSED(firstUnsignalledRetireFence);
}

void HwcTestKernel::SetExtendedModeExpectation(bool singleFullScreenVideo,
                                               bool haveSecondDisplay,
                                               uint32_t hwcFrame) {
  // Validate panel enable/disable state against shim assessment of the
  // combination of
  // 1. the inputs from MDS (is there a video state, and is there an input
  // timeout)
  // 2. whether we have full screen NV12 video
  // but only if
  // - we have a second display connected and
  // - blanking has not been requested and
  // - there is sufficient stability (i.e. long enough since a change of state).
  char strbuf[HWCVAL_DEFAULT_STRLEN];

  if ((mCrtcByDisplayIx[0] == 0) || (!mCrtcByDisplayIx[0]->IsConnected())) {
    HWCLOGW("Currently no D0. Skipping extended mode validation.");
    return;
  }

  // Do we think extended mode is called for?
  HWCLOGD_COND(eLogVideo,
               "Extended mode check: @%p Sessions %d input state %d blanking "
               "req[0] %d single full screen video %d frame:%d",
               this, mVideoSessions.size(), mInputState,
               mCrtcByDisplayIx[0]->IsBlankingRequested(),
               singleFullScreenVideo, hwcFrame);

  if (mState->IsAutoExtMode()) {
    mRequireExtendedMode = singleFullScreenVideo && haveSecondDisplay;
    mRequireEMPanel = haveSecondDisplay
                          ? HwcGetTestConfig()->GetStableModeExpect()
                          : HwcTestConfig::eOff;
    HWCLOGV_COND(eLogVideo, "mRequireEMPanel= %s",
                 HwcTestConfig::Str(mRequireEMPanel));
  } else {
    // Legacy mode:
    // Extended mode requirement calculated using inputs from MDS
    mRequireExtendedMode =
        (mVideoSessions.size() > 0) &&  // Video session(s) in progress
        singleFullScreenVideo && haveSecondDisplay;

    mRequireEMPanel = (mRequireExtendedMode && (!mInputState))
                          ? HwcTestConfig::eOff
                          : HwcTestConfig::eOn;
  }

  if ((mRequireEMPanel != mLastRequireEMPanel) ||
      mCrtcByDisplayIx[0]->IsBlankingRequested() ||
      mCrtcByDisplayIx[0]->IsDPMSInProgress()) {
    mFramesSinceEMPanelChange = 0;
    mLastRequireEMPanel = mRequireEMPanel;
  } else {
    // Wrap-around is not an issue since this will take 2 years at 60Hz.
    // Even if it does, the only effect would be some checks disabled for a few
    // frames.
    ++mFramesSinceEMPanelChange;
  }

  // Make sure that at least one display is active
  // Record the first active display, it will be used for VSync validation
  if (mCrtcByDisplayIx[0]->IsDisplayEnabled()) {
    mState->SetFirstDisplayWithVSync(0);
  } else if (mCrtcByDisplayIx[1]) {
    mState->SetFirstDisplayWithVSync(1);
  } else if (mCrtcByDisplayIx[2]) {
    mState->SetFirstDisplayWithVSync(2);
  } else {
    HWCERROR(eCheckExtendedModePanelControl,
             "Panel DPMS disabled when no other display active frame:%d",
             hwcFrame);
  }
  HWCCHECK(eCheckExtendedModePanelControl);

  // Log the power state
  HWCLOGV_COND(eLogVideo, "Display 0 power: %s",
               mCrtcByDisplayIx[0]->ReportPower(strbuf));

  HwcTestConfig::PanelModeType requireEMPanel;

  // Don't report an extended mode error if it's less than 4 frames since the
  // state changed
  if (mFramesSinceEMPanelChange > HWCVAL_EXTENDED_MODE_CHANGE_WINDOW) {
    requireEMPanel = mRequireEMPanel;
  } else {
    requireEMPanel = HwcTestConfig::eDontCare;
  }

  switch (requireEMPanel) {
    case HwcTestConfig::eOn: {
      if (!mCrtcByDisplayIx[0]->IsBlankingRequested()) {
        if ((!mCrtcByDisplayIx[0]->IsDPMSEnabled()) &&
            (!mCrtcByDisplayIx[0]->IsEsdRecoveryMode())) {
          HWCERROR(eCheckExtendedModePanelControl,
                   "Panel DPMS Disabled when extended mode not expected and "
                   "display not blanked frame:%d",
                   hwcFrame);
        }
      }

      break;
    }

    case HwcTestConfig::eOff: {
      if (mCrtcByDisplayIx[0]->IsDPMSEnabled()) {
        HWCERROR(eCheckExtendedModePanelControl,
                 "Extended mode expected but panel not DPMS disabled frame:%d",
                 hwcFrame);
      }

      break;
    }

    case HwcTestConfig::eDontCare: {
      HWCLOGD_COND(eLogVideo,
                   "Extended mode change not stable (%d frames since change) "
                   "or undefined, not validating frame:%d",
                   mFramesSinceEMPanelChange, hwcFrame);
      break;
    }

    default: { ALOG_ASSERT(0); }
  }
}

bool HwcTestKernel::BelievedEmpty(uint32_t width, uint32_t height) {
  return ((width == 16) && (height == 16));
}

void HwcTestKernel::WaitForCompValToComplete() {
  if (mCompVal.get()) {
    mCompVal->WaitUntilIdle();
  }
}

// Hwc Options
// Record the value of an HWC option in our internal cache.
// (Option values come from log parsing).
void HwcTestKernel::SetHwcOption(android::String8& optionName,
                                 android::String8& value) {
  HWCLOGD_COND(eLogParse, "Parsed HWC option %s: %s", optionName.string(),
               value.string());
  mHwcOptions.replaceValueFor(optionName, value);

  if (optionName == "dmconfig") {
    ParseDmConfig(value.string());
  }
}

// Get the value of an HWC option from our internal cache of HWC options.
const char* HwcTestKernel::GetHwcOptionStr(const char* optionName) {
  ssize_t ix = mHwcOptions.indexOfKey(android::String8(optionName));

  if (ix >= 0) {
    return mHwcOptions.valueAt(ix).string();
  }

  return 0;
}

int HwcTestKernel::GetHwcOptionInt(const char* optionName) {
  const char* str = GetHwcOptionStr(optionName);

  if (str) {
    int ret = atoi(str);
    HWCLOGD_COND(eLogOptionParse, "GetHwcOptionInt(%s) atoi(%s)=%d", optionName,
                 str, ret);
    return ret;
  } else {
    HWCLOGD_COND(eLogOptionParse, "GetHwcOptionInt(%s) no value", optionName);
    return 0;
  }
}

void HwcTestKernel::ParseDmConfig(const char* str) {
  // Parse Display configuration option value
  const char* p = str;

  while (*p) {
    if (strncmpinc(p, "[SF:") != 0)
      return;

    uint32_t logDisp = atoiinc(p);
    skipws(p);

    // All logical->physical display mappings in a single set
    // (we need to decide atomically if the set matches)
    android::Vector<Hwcval::LogDisplayMapping> mappings;
    bool matches = true;

    if (strncmpinc(p, "MOSAIC") == 0) {
      HWCLOGV_COND(eLogParse, "Matched MOSAIC leaving %s", p);
      skipws(p);

      // Parse logical display widthxheight
      uint32_t w = atoiinc(p);
      if (!ExpectChar(p, 'x'))
        return;
      uint32_t h = atoiinc(p);
      skipws(p);
      HWCLOGV_COND(eLogParse, "Matched logical size %dx%d leaving %s", w, h, p);

      if ((strncmpinc(p, "PANEL") != 0) && (strncmpinc(p, "EXTERNAL") != 0)) {
        HWCLOGV_COND(eLogParse, "ParseDmConfig: display type not matched: %s",
                     p);
        return;
      }

      skipws(p);

      while (strncmpinc(p, "(P:") == 0) {
        // Get physical display number
        uint32_t disp = atoiinc(p);
        skipws(p);
        HWCLOGV_COND(eLogParse, "Matched (P:%d leaving %s", disp, p);

        // Get flags
        uint32_t flags = atoiinc(p);
        skipws(p);
        HWCLOGV_COND(eLogParse, "Matched flags %d leaving %s", flags, p);

        // Get source crop for this physical display
        uint32_t sx = atoiinc(p);
        if (!ExpectChar(p, ','))
          return;
        uint32_t sy = atoiinc(p);
        skipws(p);
        uint32_t sw = atoiinc(p);
        if (!ExpectChar(p, 'x'))
          return;
        uint32_t sh = atoiinc(p);
        skipws(p);
        HWCLOGV_COND(eLogParse, "Matched source crop %d,%d %dx%d leaving %s",
                     sx, sy, sw, sh, p);

        // Get destination display offset
        uint32_t dx = atoiinc(p);
        if (!ExpectChar(p, ','))
          return;
        uint32_t dy = atoiinc(p);
        skipws(p);
        HWCLOGV_COND(eLogParse, "Matched dest offset %d,%d leaving %s", dx, dy,
                     p);

        // Finally the physical display size and rate we need to match
        uint32_t phx = atoiinc(p);
        HWCLOGV_COND(eLogParse, "Matched physical display width %d leaving %s",
                     phx, p);
        if (!ExpectChar(p, 'x'))
          return;
        uint32_t phy = atoiinc(p);
        HWCLOGV_COND(eLogParse, "Matched physical display height %d leaving %s",
                     phy, p);
        if (!ExpectChar(p, '@'))
          return;
        uint32_t phrate = atoiinc(p);
        HWCLOGV_COND(eLogParse,
                     "Matched physical display refresh %d leaving %s", phrate,
                     p);
        if (!ExpectChar(p, ')'))
          return;
        skipws(p);

        if ((disp >= HWCVAL_MAX_CRTCS) || (logDisp > HWCVAL_MAX_LOG_DISPLAYS)) {
          HWCLOGW("Invalid display config: bad display index SF:%d P:%d",
                  logDisp, disp);
          return;
        }
        Hwcval::LogDisplayMapping ldm(logDisp, disp, flags, sx, sy, sw, sh, dx,
                                      dy, sw, sh);
        ldm.Log("PARSED MATCHED dmconfig fragment");
        mappings.add(ldm);

        HwcTestCrtc* crtc = mCrtcByDisplayIx[disp];
        // If the physical display does not exist, no match.
        if (crtc == 0) {
          HWCLOGD_COND(eLogMosaic,
                       "ParseDmConfig: No P%d => No mapping will be set", disp);
          return;
        }

        matches &= crtc->MatchMode(phx, phy, phrate);
      }
    } else if (strncmpinc(p, "PASSTHROUGH") == 0) {
      // TODO
      while (*p && (*p != ']')) {
        p++;
      }

      // For now, if we find a passthrough display, we assume that it will match
      // to default operation
      // if nothing else does.
      //
      // This is not a safe assumption in all cases but it will do for a start.
    }

    if (*p++ != ']') {
      // mapping set incorrectly terminated
      return;
    }

    if (matches) {
      // OK, we have found the first mapping set that matches the display modes.
      MapLogDisplays(mappings);
      return;
    }
    skipws(p);
  }
}

// Tell each of the CRTCs the current logical display mapping.
void HwcTestKernel::MapLogDisplays(
    android::Vector<Hwcval::LogDisplayMapping> mappings) {
  for (uint32_t i = 0; i < mappings.size(); ++i) {
    const Hwcval::LogDisplayMapping& mapping = mappings.itemAt(i);

    uint32_t displayIx = mapping.mDisplayIx;
    HwcTestCrtc* crtc = mCrtcByDisplayIx[displayIx];
    crtc->SetDisplayMapping(mapping);
  }
}

// ************** IOCTL VALIDATION FUNCTIONS ********************
// As these can happen at pretty much any time, it is important that we don't
// lock our mutex for these as this can lead to deadlocks.
//
// So instead, we create a work item and push it to our work queue.
// These get processed next time some function places the lock.
//
void HwcTestKernel::CheckIoctlGemOpen(int fd, struct drm_gem_open* gemOpen) {
  mWorkQueue.Push(
      new Hwcval::Work::GemOpenItem(fd, gemOpen->name, gemOpen->handle));
}

void HwcTestKernel::CheckIoctlGemClose(int fd, struct drm_gem_close* gemClose) {
  mWorkQueue.Push(new Hwcval::Work::GemCloseItem(fd, gemClose->handle));
}

void HwcTestKernel::CheckIoctlGemFlink(int fd, struct drm_gem_flink* flink) {
  mWorkQueue.Push(
      new Hwcval::Work::GemFlinkItem(fd, flink->name, flink->handle));
}

void HwcTestKernel::CheckIoctlGemWait(int fd, struct drm_i915_gem_wait* gemWait,
                                      int status, int64_t delayNs) {
  mWorkQueue.Push(
      new Hwcval::Work::GemWaitItem(fd, gemWait->bo_handle, status, delayNs));

  // Optionally stall the gem thread
  DoStall(Hwcval::eStallGemWait);
}

void HwcTestKernel::CheckIoctlGemCreate(int fd,
                                        struct drm_i915_gem_create* gemCreate) {
  mWorkQueue.Push(new Hwcval::Work::GemCreateItem(fd, gemCreate->handle));
}

void HwcTestKernel::CheckIoctlPrime(int fd, struct drm_prime_handle* prime) {
  mWorkQueue.Push(new Hwcval::Work::PrimeItem(fd, prime->handle, prime->fd));
}

// ************** WORK QUEUE ACTION FUNCTIONS ****************
//
// Action for buffer free instruction.
// This originates from a log entry not an IOCTL, as it is HWC that knows (via a
// notification from gralloc) when a buffer is deallocated.
//
void HwcTestKernel::DoBufferFree(const Hwcval::Work::BufferFreeItem& item) {
  char strbuf[HWCVAL_DEFAULT_STRLEN];
  HWCLOGD_COND(eLogBuffer, "Processing buffer free item for handle %p",
               item.mHandle);

  ssize_t ix = mBuffers.indexOfKey(item.mHandle);

  if (ix >= 0) {
    android::sp<DrmShimBuffer> buf = mBuffers.valueAt(ix);

    HWCLOGD_COND(eLogBuffer, "Removing %s", buf->IdStr(strbuf));

    HwcTestBufferObjectVector& bos = buf->GetBos();
    for (uint32_t i = 0; i < bos.size(); ++i) {
      android::sp<HwcTestBufferObject> bo = bos.itemAt(i);
      BoKey k = {bo->mFd, bo->mBoHandle};
      mBosByBoHandle.removeItem(k);
    }

    mBuffers.removeItemsAt(ix);
  } else {
    HWCLOGI("Attempt to remove unknown buffer handle %p from mBuffers",
            item.mHandle);
  }
}

// Process an Open work item, which is the record of an
// IOCTL_GEM_OPEN request.
//
// This is the request to open a buffer, specified by name
// (historically) or PRIME, and associate it with a buffer object handle.
//
void HwcTestKernel::DoGem(const Hwcval::Work::GemOpenItem& item) {
  char str[HWCVAL_DEFAULT_STRLEN];
  char str2[HWCVAL_DEFAULT_STRLEN];
  BoKey k = {item.mFd, item.mBoHandle};

  if (item.mBoHandle) {
    ssize_t ix = mBuffersByGlobalId.indexOfKey(item.mId);
    android::sp<DrmShimBuffer> buf;
    android::sp<HwcTestBufferObject> bo;

    // Find any buffer that maps to the global ID (PRIME/name).
    if (ix >= 0) {
      buf = mBuffersByGlobalId.valueAt(ix);
    }

    ssize_t boIx = mBosByBoHandle.indexOfKey(k);
    if (boIx >= 0) {
      // We've found our buffer object record for this bo handle.
      // Get the DrmShimBuffer it points to - does it already map to this
      // global ID?
      bo = mBosByBoHandle.valueAt(boIx);
      android::sp<DrmShimBuffer> oldBuf = bo->mBuf.promote();

      if (oldBuf.get()) {
        if (oldBuf->GetGlobalId() == item.mId) {
          // Yes - nothing to do
          HWCLOGD_COND(eLogBuffer,
                       "IOCTL_GEM_OPEN: %s already associated with buf %s",
                       bo->IdStr(str), oldBuf->IdStr(str2));
          ValidateBo(oldBuf, "IOCTL_GEM_OPEN already associated");
          return;
        } else {
          // No - so (a) the old DrmShimBuffer no longer maps to this bo (buffer
          // object)
          HWCLOGD_COND(eLogBuffer,
                       "IOCTL_GEM_OPEN: %s presently associated with buf %s "
                       "(new " BUFIDSTR " 0x%x), removing",
                       bo->IdStr(str), oldBuf->IdStr(str2), item.mId);
          oldBuf->RemoveBo(bo);

          //          (b) the DrmShimBuffer for our global ID does now map to
          //          this bo.
          if (buf.get()) {
            buf->AddBo(bo);
            bo->mBuf = buf;
            HWCLOGD("IOCTL_GEM_OPEN: Now %s", buf->IdStr(str));
            ValidateBo(buf, "IOCTL_GEM_OPEN association moved (dst)");
            ValidateBo(oldBuf, "IOCTL_GEM_OPEN association moved (src)");
            return;
          }
        }
      }
    }

    // If we get this far, it's normally because we don't know anything about
    // this bo handle.
    if (ix >= 0) {
      // We know the global ID, so create the bo record and attach it to the
      // DrmShimBuffer
      // for that global ID.
      if (bo.get() == 0) {
        bo = CreateBufferObject(item.mFd, item.mBoHandle);
        ALOG_ASSERT(bo->mBuf.promote() == 0);
      }

      bo->mBuf = buf;
      buf->AddBo(bo);

      HWCLOGD_COND(eLogBuffer,
                   "IOCTL_GEM_OPEN " BUFIDSTR " 0x%x -> fd %d boHandle 0x%x %s",
                   item.mId, item.mFd, item.mBoHandle, buf->IdStr(str));

      if (boIx < 0) {
        mBosByBoHandle.add(k, bo);
      }

      ValidateBo(buf, "IOCTL_GEM_OPEN transfer target");
    } else {
      // We don't know the bo handle OR the global ID.
      // We are recording the opening of a buffer we haven't seen before, we
      // expect to
      // get more information on it later when RecordBufferState is called.
      buf = new DrmShimBuffer(0);
      buf->SetGlobalId(item.mId)->SetLastHwcFrame(mFN);

      // Assume this is a blanking buffer until it is associated with a handle.
      buf->SetBlanking(true);
      bo = CreateBufferObject(item.mFd, item.mBoHandle);
      bo->mBuf = buf;
      buf->AddBo(bo);

      if (boIx >= 0) {
        mBosByBoHandle.replaceValueAt(boIx, bo);
      } else {
        mBosByBoHandle.add(k, bo);
      }

      MapGlobalId(item.mId, buf);

      HWCLOGD_COND(eLogBuffer,
                   "IOCTL_GEM_OPEN " BUFIDSTR
                   " 0x%x -> fd %d boHandle 0x%x handle NOT YET ALLOCATED",
                   item.mId, item.mFd, item.mBoHandle);
      ValidateBo(buf, "IOCTL_GEM_OPEN NOT PREVIOUSLY ALLOCATED");
    }

    ValidateBo(bo, 0, "IOCTL_GEM_OPEN");
  }
}

// Process a Close work item
// This records that a buffer object has been closed
// though owing to reference counting on the underlying object
// we may not necessarily be able to throw the reference away immediately.
void HwcTestKernel::DoGem(const Hwcval::Work::GemCloseItem& item) {
  BoKey k = {item.mFd, item.mBoHandle};
  RemoveBo(k, "IOCTL_GEM_CLOSE");
}

//
// Process an Flink work item, which is the record of an IOCTL_GEM_FLINK
// request.
//
// Note: This function has been built up over time and handles a number of
// special cases.
//
// I'm not going to bother to document them because under the change to
// using PRIMEs rather than buffer names, IOCTL_GEM_FLINK will no longer
// be used.
//
void HwcTestKernel::DoGem(const Hwcval::Work::GemFlinkItem& item) {
  BoKey k = {item.mFd, item.mBoHandle};
  ssize_t boHandleIx = mBosByBoHandle.indexOfKey(k);
  ssize_t ix = mBuffersByGlobalId.indexOfKey(item.mId);
  char strbuf[HWCVAL_DEFAULT_STRLEN];

  if (boHandleIx >= 0) {
    android::sp<HwcTestBufferObject> bo = mBosByBoHandle.valueAt(boHandleIx);
    android::sp<DrmShimBuffer> buf = bo->mBuf.promote();

    if (buf.get() == 0) {
      if (ix >= 0) {
        buf = mBuffersByGlobalId.valueAt(ix);

        int globalId = buf->GetCurrentGlobalId();
        if (globalId != item.mId) {
          ix = -1;
        }
      }

      if (ix >= 0) {
        if (buf->GetGlobalId() != item.mId) {
          ValidateBo(buf, "flink old");
          buf->RemoveBo(bo);
          ValidateBo(buf, "flink old after remove");
          buf = new DrmShimBuffer(0);
          buf->SetBlanking(true);
          MapGlobalId(item.mId, buf);
        } else {
          HWCLOGD_COND(eLogBuffer,
                       "Flink: @%p boHandle 0x%x already associated to %s",
                       bo.get(), item.mBoHandle, buf->IdStr(strbuf));

          buf->RemoveBo(bo->mFd, bo->mBoHandle);
          buf->AddBo(bo);
          bo->mBuf = buf;
          MapGlobalId(item.mId, buf);

          // Remove any bos from the buffer which
          // no longer belong to it
          HwcTestBufferObjectVector& bos = buf->GetBos();
          for (int i = bos.size() - 1; i >= 0; --i) {
            android::sp<HwcTestBufferObject> lbo = bos.itemAt(i);
            BoKey kk = {lbo->mFd, lbo->mBoHandle};
            ssize_t ix = mBosByBoHandle.indexOfKey(kk);

            if (ix >= 0) {
              android::sp<HwcTestBufferObject> gbo = mBosByBoHandle.valueAt(ix);

              if (gbo != lbo) {
                bos.removeAt(i);
              }
            } else {
              bos.removeAt(i);
            }
          }

          ValidateBo(bo, 0, "Flink already associated");
          ValidateBo(buf, "Flink already associated");
          return;
        }
      } else {
        buf = new DrmShimBuffer(0);
        buf->SetBlanking(true);
        MapGlobalId(item.mId, buf);
      }

      buf->SetGlobalId(item.mId);
      buf->AddBo(bo);
      bo->mBuf = buf;
      HWCLOGD_COND(
          eLogBuffer, "IOCTL_GEM_FLINK: Linked " BUFIDSTR
                      " 0x%x in new buf@%p to fd %d boHandle 0x%x handle %p",
          item.mId, buf.get(), item.mFd, item.mBoHandle, buf->GetHandle());
    } else if (buf->GetGlobalId() == 0) {
      // Normal case: We are linking an open buffer object to a name when it
      // previously had none.
      MapGlobalId(item.mId, buf);
      HWCLOGD_COND(eLogBuffer, "IOCTL_GEM_FLINK: Linked " BUFIDSTR
                               " 0x%x to fd %d boHandle 0x%x handle %p",
                   item.mId, item.mFd, item.mBoHandle, buf->GetHandle());
    } else if (buf->GetGlobalId() != item.mId) {
      // We are requested to link an open buffer object to a name even though it
      // prevously had one.
      // Let's assume that the old bo should be closed and a new one should be
      // opened.
      buf->RemoveBo(bo);
      buf = new DrmShimBuffer(0);

      buf->AddBo(bo);
      bo->mBuf = buf;

      MapGlobalId(item.mId, buf);
      buf->SetBlanking(true);
      HWCLOGD_COND(eLogBuffer,
                   "IOCTL_GEM_FLINK: boHandle 0x%x id has changed so we have "
                   "created it and linked it to " BUFIDSTR " 0x%x",
                   item.mBoHandle, item.mId);
    }

    HWCLOGV_COND(eLogBuffer, "IOCTL_GEM_FLINK: %s", buf->IdStr(strbuf));
    ValidateBo(buf, "IOCTL_GEM_FLINK");
  } else {
    // bo unknown. So we assume we must open it.
    // I don't really understand why this is happening, but it is!
    // Reading the struct definition, it appears that this call is invoked for
    // the system to give a name to an already open buffer.
    // But we find that in some cases, the boHandle is one that has not been
    // opened.

    if (ix >= 0) {
      android::sp<DrmShimBuffer> oldBuf = mBuffersByGlobalId.valueAt(ix);

      HWCLOGD_COND(eLogBuffer,
                   "IOCTL_GEM_FLINK: Name 0x%x is new buffer, remove old "
                   "association to handle %p (now linked to fd %d boHandle "
                   "0x%x)",
                   item.mId, oldBuf->GetHandle(), item.mFd, item.mBoHandle);

      // Remove buffer from name and handle indices
      mBuffersByGlobalId.removeItemsAt(ix);
      mBuffers.removeItem(oldBuf->GetHandle());
    } else {
      HWCLOGD_COND(
          eLogBuffer,
          "IOCTL_GEM_FLINK: fd %d boHandle 0x%x and name 0x%x not known",
          item.mFd, item.mBoHandle, item.mId);
    }

    // Create a new association.
    android::sp<DrmShimBuffer> buf = new DrmShimBuffer(0);
    buf->SetGlobalId(item.mId)->SetLastHwcFrame(mFN);

    // Assume this is a blanking buffer until it is associated with a handle.
    buf->SetBlanking(true);

    android::sp<HwcTestBufferObject> bo =
        CreateBufferObject(item.mFd, item.mBoHandle);
    buf->AddBo(bo);
    bo->mBuf = buf;

    mBuffersByGlobalId.add(item.mId, buf);
    BoKey k = {item.mFd, item.mBoHandle};
    mBosByBoHandle.add(k, bo);
    HWCLOGV_COND(eLogBuffer,
                 "IOCTL_GEM_FLINK name 0x%x -> fd %d boHandle 0x%x handle NOT "
                 "YET ALLOCATED",
                 item.mId, item.mFd, item.mBoHandle);
    HWCLOGV_COND(eLogBuffer, "IOCTL_GEM_FLINK: %s", buf->IdStr(strbuf));
  }
}

// Process a Create work item
// This is a record of a IOCTL_I915_GEM_CREATE request which creates
// a buffer object, without associating it with any name/PRIME.
void HwcTestKernel::DoGem(const Hwcval::Work::GemCreateItem& item) {
  char str[HWCVAL_DEFAULT_STRLEN];
  BoKey k = {item.mFd, item.mBoHandle};

  ssize_t ix = mBosByBoHandle.indexOfKey(k);

  if (ix < 0) {
    // This genuinely is a new object, we don't know this bo handle.
    android::sp<DrmShimBuffer> buf;
    android::sp<HwcTestBufferObject> bo;

    // Create a dummy DrmShimBuffer, this will get more information later
    // when RecordBufferState is called.
    buf = new DrmShimBuffer(0);

    // Assume this is a blanking buffer until it is associated with a handle.
    buf->SetBlanking(true);

    // Create the bo record and associate it with the DrmShimBuffer.
    bo = CreateBufferObject(item.mFd, item.mBoHandle);
    bo->mBuf = buf;
    buf->AddBo(bo);

    mBosByBoHandle.add(k, bo);
    HWCLOGD_COND(eLogBuffer, "IOCTL_I915_GEM_CREATE fd %d boHandle 0x%x: %s",
                 item.mFd, item.mBoHandle, buf->IdStr(str));
    ValidateBo(buf, "IOCTL_I915_GEM_CREATE");
  } else {
    android::sp<HwcTestBufferObject> bo = mBosByBoHandle.valueAt(ix);
    HWCLOGD_COND(eLogBuffer, "IOCTL_I915_GEM_CREATE bo already exists: %s",
                 bo->IdStr(str));
  }
}

// Process the work item for a TIMEOUT of a GEM WAIT.
// No record is made of a successful GEM WAIT, as we don't require this
// information.
//
// A GEM WAIT is an IOCTL that is requesting to wait on a target buffer until
// composition is complete.
// So a timeout on one of these is likely to mean that iVP (usually) or perhaps
// partitioned composer
// has locked up.
void HwcTestKernel::DoGem(const Hwcval::Work::GemWaitItem& item) {
  char str[HWCVAL_DEFAULT_STRLEN];
  BoKey k = {item.mFd, item.mBoHandle};

  if (item.mBoHandle) {
    ssize_t boHandleIx = mBosByBoHandle.indexOfKey(k);

    if (boHandleIx >= 0) {
      android::sp<HwcTestBufferObject> bo = mBosByBoHandle.valueAt(boHandleIx);
      android::sp<DrmShimBuffer> buf = bo->mBuf.promote();

      if (buf.get()) {
        HWCERROR(
            eCheckDrmIoctlGemWaitLatency,
            "Timeout %fs waiting for %s boHandle 0x%x buffer %s (error %d)",
            double(item.mDelayNs) / 1000000000.0, buf->GetSourceName(),
            item.mBoHandle, buf->IdStr(str), item.mStatus);
      } else {
        HWCERROR(eCheckDrmIoctlGemWaitLatency,
                 "Timeout %fs waiting for buffer object %s (unknown buffer, "
                 "error %d)",
                 double(item.mDelayNs) / 1000000000.0, bo->IdStr(str),
                 item.mStatus);
      }
    } else {
      HWCERROR(eCheckDrmIoctlGemWaitLatency,
               "Timeout %fs waiting for unknown boHandle 0x%x",
               double(item.mDelayNs) / 1000000000.0, item.mBoHandle);
    }
  }
}

// Remove a buffer object from our internal tracking and update the data
// structures accordingly.
void HwcTestKernel::RemoveBo(BoKey k, const char* str) {
  char strbuf[HWCVAL_DEFAULT_STRLEN];

  ssize_t ix = mBosByBoHandle.indexOfKey(k);

  if (ix >= 0) {
    android::sp<HwcTestBufferObject> bo = mBosByBoHandle.valueAt(ix);
    android::sp<DrmShimBuffer> buf = bo->mBuf.promote();

    if (buf.get()) {
      buf->RemoveBo(bo);

      if (buf->GetBos().size() == 0) {
        // All buffer objects referring to this buffer have been removed.
        // So next time we see this buffer, we assume it is a new one.
        HWCLOGD_COND(eLogBuffer,
                     "IOCTL_GEM_CLOSE: removed association from " BUFIDSTR
                     " 0x%x to buf@%p handle %p",
                     buf->GetGlobalId(), buf.get(), buf->GetHandle());
        mBuffersByGlobalId.removeItem(buf->GetGlobalId());
        buf->SetGlobalId(0);
      }
    }

    mBosByBoHandle.removeItem(k);

    HWCLOGD_COND(eLogBuffer, "%s: Closed fd %d boHandle 0x%x %s", str, k.fd,
                 k.h, buf.get() ? buf->IdStr(strbuf) : "");
    ValidateBo(buf, str);

  } else {
    // This does happen - boHandles get closed when they have already been
    // closed.
    // Presumably this is harmless.
    HWCLOGW_COND(eLogBuffer, "%s: fd %d boHandle 0x%x unknown", str, k.fd, k.h);
  }
}

// Under ADF, we required tracking of PRIME IOCTLs. This is no longer needed.
void HwcTestKernel::DoPrime(const Hwcval::Work::PrimeItem& item) {
  HWCVAL_UNUSED(item);
}

#ifdef HWCVAL_INTERNAL_BO_VALIDATION
// This is aggressive validation of our internal buffer tracking
// and should only be used if this is going wrong - for example there appears to
// be confusion
// as to which buffer is which, or SEGVs are occurring in this area.
//
// This should not be necessary unless there is a spec change in gralloc, GEM,
// or the way
// these are used.
void HwcTestKernel::ValidateBo(android::sp<DrmShimBuffer> buf,
                               const char* str) {
  char strbuf[HWCVAL_DEFAULT_STRLEN];

  if (buf.get()) {
    int id = buf->GetGlobalId();

    if (id > 0) {
      ssize_t ix = mBuffersByGlobalId.indexOfKey(id);
      if (ix < 0) {
        HWCERROR(eCheckInternalError, "Name not indexed: %s",
                 buf->IdStr(strbuf));
        BoValidationAssert();
      }
    }

    HwcTestBufferObjectVector& bos = buf->GetBos();
    for (uint32_t i = 0; i < bos.size(); ++i) {
      android::sp<HwcTestBufferObject> bo = bos.itemAt(i);
      ValidateBo(bo, buf, str);
    }
  }
}

void HwcTestKernel::ValidateBo(android::sp<HwcTestBufferObject> bo,
                               android::sp<DrmShimBuffer> buf,
                               const char* str) {
  if (bo.get()) {
    char strbuf[HWCVAL_DEFAULT_STRLEN];
    char strbuf2[HWCVAL_DEFAULT_STRLEN];
    BoKey k = {bo->mFd, bo->mBoHandle};
    android::sp<DrmShimBuffer> parentBuf = bo->mBuf.promote();

    if (parentBuf.get()) {
      HwcTestBufferObjectVector& bos = parentBuf->GetBos();
      ssize_t boIx = bos.indexOf(bo);
      if (boIx < 0) {
        HWCERROR(eCheckTestBufferAlloc, "%s: BO %s missing ref from %s", str,
                 bo->IdStr(strbuf), parentBuf->IdStr(strbuf2));
        BoValidationAssert();
      }
    }

    ssize_t ix = mBosByBoHandle.indexOfKey(k);

    if (ix < 0) {
      HWCERROR(eCheckTestBufferAlloc, "%s: BO %s not indexed", str,
               bo->IdStr(strbuf));
      if (buf.get()) {
        HWCLOGV_COND(eLogBuffer, "  -- %s", buf->IdStr(strbuf));
      }

      BoValidationAssert();
    } else {
      android::sp<HwcTestBufferObject> otherBo = mBosByBoHandle.valueAt(ix);
      if (bo != otherBo) {
        HWCERROR(eCheckTestBufferAlloc, "%s: BO %s not indexed, instead %s",
                 str, bo->IdStr(strbuf), otherBo->IdStr(strbuf2));
        if (buf.get()) {
          HWCLOGV_COND(eLogBuffer, "  -- %s", buf->IdStr(strbuf));
          BoValidationAssert();
        }
      }

      if (buf.get()) {
        if (parentBuf != buf) {
          HWCERROR(eCheckTestBufferAlloc,
                   "%s: BO %s does not point to parent %s", str,
                   bo->IdStr(strbuf), buf->IdStr(strbuf2));
          if (parentBuf.get()) {
            HWCLOGV_COND(eLogBuffer, "  -- instead %s",
                         parentBuf->IdStr(strbuf));
            BoValidationAssert();
          }
        }
      }
    }
  }
}
#endif  // INTERNAL_BO_VALIDATION

// This is the main function for recording the state of a buffer where we know
// its gralloc handle.
// This includes its interception from SF/the harness (i.e. CheckSetEnter) and
// also interception of compositions.
//
// The caller indicates the source of the buffer so we can distinguish between
// SF inputs, composition and other cases.
android::sp<DrmShimBuffer> HwcTestKernel::RecordBufferState(
    HWCNativeHandle handle, Hwcval::BufferSourceType bufferSource,
    char* notes) {
  ATRACE_CALL();
  mWorkQueue.Process();
  notes[0] = '\0';
  int notesLen = 0;
  bool isOnSet;

  android::sp<DrmShimBuffer> buf;
  android::sp<DrmShimBuffer> existingBuf;
  char strbuf[HWCVAL_DEFAULT_STRLEN];
#if 0

  HWCLOGD_COND(eLogBuffer, "HwcTestKernel::RecordBufferState Enter handle %p",
               handle->handle_);
  // info needed at various points in this function.
  isOnSet = ((bufferSource == Hwcval::BufferSourceType::Input) ||
             (bufferSource == Hwcval::BufferSourceType::SfComp));

#if INTEL_UFO_GRALLOC_HAVE_PRIME
  uint32_t boHandle;
  mGralloc.getBufferObject(handle, &boHandle);
  android::sp<HwcTestBufferObject> bo = GetBufferObject(boHandle);
  android::sp<DrmShimBuffer> boBuf;

  HWCLOGD_COND(eLogBuffer,
               "HwcTestKernel::RecordBufferState got bo@%p handle %p", bo.get(),
               handle);
  if (bo.get()) {
    boBuf = bo->mBuf.promote();
    HWCLOGD_COND(eLogBuffer,
                 "HwcTestKernel::RecordBufferState got buf@%p bo@%p handle %p",
                 boBuf.get(), bo.get(), handle);
  }
#endif

  // For buffers appearing in the SF layer list only, we need to consider if
  // this is the
  // re-appearance of a buffer that was in the previous frame's layer list.
  // If so, we assume it is the same content and hence can be represented by the
  // same
  // DrmShimBuffer. Otherwise (even if it is the same buffer handle as a buffer
  // that
  // appeared in the layer list 2 frames ago) it is assumed to be different
  // content and
  // hence will be represented by a different DrmShimBuffer.
  //
  // For buffers created by iVP on the other hand, we assume that each time
  // there is a composition
  // we are creating new content as HWC will not repeat a composition that it
  // has already
  // done.
  ssize_t ix = mBuffers.indexOfKey(handle);

  if (ix >= 0) {
    buf = mBuffers.valueAt(ix);
    buf->ResetAppearanceCount();  // Reset consecutive appearance count to zero.
    HWCLOGD_COND(
        eLogBuffer,
        "RecordBufferState: Existing buf@%p found in mBuffers, handle %s",
        buf.get(), handle);

    if ((bufferSource == Hwcval::BufferSourceType::Input) ||
        (bufferSource == Hwcval::BufferSourceType::SfComp)) {
      // was buffer seen last frame...
      if (buf->IsCurrent(mFN) &&
          // ... and was it in the OnSet (buffer can't transition from being a
          // composition target
          // to being in the OnSet layer list - if it appears to, it is a
          // different buffer with
          // the same handle
          ((buf->GetSource() == Hwcval::BufferSourceType::Input) ||
           (buf->GetSource() == Hwcval::BufferSourceType::SfComp))) {
        notesLen += snprintf(notes + notesLen, HWCVAL_DEFAULT_STRLEN - notesLen,
                             "LASTFRAME ");

// If the buffer has the same name as the one we know
#if INTEL_UFO_GRALLOC_HAVE_PRIME
        if (((bi.NAME_OR_PRIME == buf->GetGlobalId()) ||
             (buf->GetGlobalId() == 0)) &&
            ((boBuf == buf) || (boBuf == 0)))
#else
        if (bi.NAME_OR_PRIME == buf->GetGlobalId())
#endif
        {
          notesLen +=
              snprintf(notes + notesLen, HWCVAL_DEFAULT_STRLEN - notesLen,
                       "UNCHANGED" BUFIDSTR);

          if (bi.NAME_OR_PRIME != buf->GetGlobalId()) {
            MapGlobalId(bi.NAME_OR_PRIME, buf);
          }

          // We are happy that this is the reappearance of the same buffer
          buf->SetCompositionTarget(bufferSource)
              ->SetDetails(bi)
              ->IncAppearanceCount()
              ->SetUsed(false);

          // Refresh the sequence number
          buf->SetLastHwcFrame(mFN, true);

#if INTEL_UFO_GRALLOC_HAVE_PRIME
          if (bo.get()) {
            bo->mBuf = buf;
            buf->AddBo(bo);
            HWCLOGD_COND(eLogBuffer,
                         "HwcTestKernel::RecordBufferState adding bo@%p to "
                         "buf@ handle %p",
                         bo.get(), buf.get(), handle);
          }
#endif
          // Hence, we are returning the existing DrmShimBuffer record.
          HWCLOGD_COND(eLogBuffer,
                       "RecordBufferState: Exit buf@%p IdStr: " BUFIDSTR " %s",
                       buf.get(), buf->IdStr(strbuf));
          return buf;
        } else {
          notesLen +=
              snprintf(notes + notesLen, HWCVAL_DEFAULT_STRLEN - notesLen,
                       "DIFFERENT" BUFIDSTR);
        }
      } else {
        // Because this buffer has not been seen sufficiently recently, we have
        // decided to treat this as new
        // content.
        // Therefore, we will drop through to the code that will create a new
        // DrmShimBuffer.
        // We remember what the existing DrmShimBuffer is so we can transfer the
        // FB IDs and buffer objects
        // to the new DrmShimBuffer.
        notesLen += snprintf(notes + notesLen, HWCVAL_DEFAULT_STRLEN - notesLen,
                             "NEWCONTENT (%s open %d new %d)",
                             buf->GetHwcFrameStr(strbuf), buf->GetOpenCount(),
                             buf->IsNew());
        buf->SetBlanking(false);
        existingBuf = buf;
        HWCLOGD_COND(
            eLogBuffer,
            "RecordBufferState: Existing buf@%p is too old for handle %s",
            buf.get(), handle);
      }
    } else  // buffer is a composition target
    {
      for (uint32_t i = 0; i < mPlanes.size(); ++i) {
        DrmShimPlane* plane = mPlanes.valueAt(i);

        HWCCHECK(eCheckCompToDisplayedBuf);
        if (plane->IsUsing(buf)) {
          HWCERROR(eCheckCompToDisplayedBuf, "%s is currently on plane %d",
                   buf->IdStr(strbuf), plane->GetPlaneId());
        }
      }
      HWCLOGD_COND(
          eLogBuffer,
          "RecordBufferState: Existing buf@%p is composition target %s",
          buf.get(), handle);
    }
  }

#if !INTEL_UFO_GRALLOC_HAVE_PRIME
  ssize_t nameIx = mBuffersByGlobalId.indexOfKey(bi.NAME_OR_PRIME);

  if (nameIx >= 0) {
    // We found the (global) buffer name/prime. So a GEM OPEN must have been
    // called...
    android::sp<DrmShimBuffer> altbuf = mBuffersByGlobalId.valueAt(nameIx);
    altbuf->ResetAppearanceCount();  // Reset consecutive appearance count to
                                     // zero.
    HWCLOGD_COND(eLogBuffer,
                 "RecordBufferState: Existing buf@%p found in "
                 "mBuffersByGlobalId, handle %s",
                 altbuf.get(), handle);

    if (altbuf->GetHandle() == 0) {
      // The buffer name/prime has not yet been associated with a handle, so we
      // can do that now.
      altbuf->SetCompositionTarget(bufferSource)
          ->SetDetails(bi)
          ->SetHandle(handle)
          ->SetBlanking(false);

      // Refresh the sequence number
      // altbuf->SetLastHwcFrame(mHwcFrame, isOnSet);

      // Add to the handle index
      mBuffers.replaceValueFor(handle, altbuf);

      HWCLOGD_COND(eLogBuffer,
                   "RecordBufferState: Exit buf@%p IdStr: " BUFIDSTR " %s",
                   altbuf.get(), altbuf->IdStr(strbuf));

      return altbuf;
    } else if (altbuf.get() != existingBuf.get()) {
      HWCLOGD_COND(
          eLogBuffer, "RecordBufferState: existing buf@%p handle %p: " BUFIDSTR
                      " 0x%x presently associated with DIFFERENT %s",
          altbuf.get(), handle, bi.NAME_OR_PRIME, altbuf->IdStr(strbuf));

      existingBuf = altbuf;
    }
  } else {
    notesLen += snprintf(notes, HWCVAL_DEFAULT_STRLEN - notesLen, "BRANDNEW ");
  }
#endif  // !INTEL_UFO_GRALLOC_HAVE_PRIME

  // Buffer not known to us
  // Create a new record to track it
  buf = new DrmShimBuffer(handle, bufferSource);

  // Add or update current record for this buffer handle
  mBuffers.replaceValueFor(handle, buf);

  HWCLOGD_COND(eLogBuffer, "RecordBufferState: new buf@%p handle %p: ",
               buf.get(), handle);

#if INTEL_UFO_GRALLOC_HAVE_PRIME
  boBuf = bo->mBuf.promote();
  HWCLOGD_COND(eLogBuffer,
               "RecordBufferState: Checking boBuf buf@%p bo@%p vs buf@%p",
               boBuf.get(), bo.get(), buf.get());

  // Fix up links between buffer and buffer object in the case where these have
  // changed.
  if (boBuf != buf) {
    if (boBuf.get() != 0) {
      boBuf->RemoveBo(bo);
      HWCLOGD_COND(eLogBuffer, "RecordBufferState: Removed bo@%p from buf@%p",
                   bo.get(), boBuf.get());

      if ((existingBuf.get() == 0) || (existingBuf->GetBos().size() == 0) ||
          ((existingBuf->NumFbIds() == 0) && (boBuf->NumFbIds() > 0))) {
        existingBuf = boBuf;
        HWCLOGD_COND(
            eLogBuffer,
            "RecordBufferState: Using boBuf buf@%p as existing buf@%pIdStr",
            boBuf.get(), existingBuf.get());
      }
    }

    bo->mBuf = buf;
    buf->AddBo(bo);
    HWCLOGD_COND(eLogBuffer, "RecordBufferState: Added bo@%p from buf@%p",
                 bo.get(), buf.get());
  }
#endif

// Transfer indices from old buffer instance to new one.
#if INTEL_UFO_GRALLOC_HAVE_PRIME
  if (existingBuf.get())
#else
  if (existingBuf.get() && (existingBuf->GetGlobalId() == bi.NAME_OR_PRIME))
#endif
  {
    HWCLOGD_COND(eLogBuffer, "RecordBufferState: %s -> buf@%p handle %p",
                 existingBuf->IdStr(strbuf), buf.get(), buf->GetHandle());

    // Transfer buffer objects from existing buffer to new one
    // and at the same time, give the existing buffer (which is now historical)
    // a copy.
    // The reason why the new buffer must have the original buffer objects is
    // that it
    // represents the current state, and that is what must be located when a Drm
    // call is made.
    HwcTestBufferObjectVector bosForOldBuf;
    buf->GetBos() = existingBuf->GetBos();
    HWCLOGD_COND(eLogBuffer, "RecordBufferState: buf@%p takes bos from buf@%p",
                 buf.get(), existingBuf.get());

    for (uint32_t i = 0; i < buf->GetBos().size(); ++i) {
      android::sp<HwcTestBufferObject> existingBo =
          existingBuf->GetBos().itemAt(i);
      android::sp<HwcTestBufferObject> newBo = existingBo->Dup();
      bosForOldBuf.add(newBo);
      HWCLOGD_COND(eLogBuffer,
                   "RecordBufferState: existing bo@%p copied to bo@%p",
                   existingBo.get(), newBo.get());

      // Move existing buffer objects to the new DrmShimBuffer
      existingBo->mBuf = buf;
    }

    existingBuf->GetBos() = bosForOldBuf;
    HWCLOGD_COND(eLogBuffer, "RecordBufferState: buf@%p takes copied bos",
                 existingBuf.get());

    // Also move the FB IDs to the new DrmShimBuffer
    MoveDsIds(existingBuf, buf);
  }

  // Add or update the current record for this buffer name/prime
  MapGlobalId(bi.NAME_OR_PRIME, buf);
  ValidateBo(buf, "RecordBufferState");

  // Store FB ID and buffer sizes
  buf->SetDetails(bi)->SetLastHwcFrame(mFN, isOnSet)->SetUsed(false);

  if (HWCCOND(eLogBuffer)) {
    buf->ReportStatus(ANDROID_LOG_VERBOSE, "RecordBufferState New");
  }

  HWCLOGD_COND(eLogBuffer,
               "HwcTestKernel::RecordBufferState Exit buf@%p handle %p",
               buf.get(), handle);

#endif
  return buf;
}

void HwcTestKernel::AddZOrder(uint32_t order, uint32_t seq,
                              uint32_t planeOffset) {
  // order is the Z-order value passed to DRM
  // seq is the z-order logical sequence we want to add for plane (CRTC +
  // planeOffset)

  ssize_t ix = mOrders.indexOfKey(order);
  HwcTestCrtc::SeqVector* seqvec;

  if (ix < 0) {
    // Create the plane sequence
    seqvec = new HwcTestCrtc::SeqVector();
    mOrders.add(order, seqvec);
  } else {
    seqvec = mOrders.valueAt(ix);
  }

  if (seqvec->size() <= planeOffset) {
    seqvec->resize(planeOffset + 1);
  }
  seqvec->replaceAt(seq, planeOffset);  // Note index is second parameter
}

// Copy frame counts to HwcTestResult, ready for sending to the test.
void HwcTestKernel::SendFrameCounts(bool clear) {
  HWCVAL_LOCK(_l, mMutex);
  mWorkQueue.Process();

  for (uint32_t i = 0; i < HWCVAL_MAX_CRTCS; ++i) {
    if (mCrtcByDisplayIx[i]) {
      HwcTestResult::PerDisplay* resultPerDisp =
          HwcGetTestResult()->mPerDisplay + i;
      uint32_t dropped;
      uint32_t consecutive;
      mCrtcByDisplayIx[i]->GetDroppedFrameCounts(dropped, consecutive, clear);
      resultPerDisp->mDroppedFrameCount = dropped;
      resultPerDisp->mMaxConsecutiveDroppedFrameCount = consecutive;
      resultPerDisp->mFrameCount = mFN[i] - mStartFN[i];

      HWCCHECK(eCheckTooManyDroppedFrames);

      if (resultPerDisp->mFrameCount > 50) {
        if (dropped > (resultPerDisp->mFrameCount / 2)) {
          HWCERROR(eCheckTooManyDroppedFrames,
                   "Display %d had %d frames dropped out of %d (%d%%)", i,
                   dropped, resultPerDisp->mFrameCount,
                   (100 * dropped) / resultPerDisp->mFrameCount);
        }
      }
      // For now, this is a very generous limit.
      // TODO: Something cleverer, by allowing dropped frames only when there is
      // a good reason
      // such as being in the process of setting display resolution.
      HWCCHECK(eCheckTooManyConsecutiveDroppedFrames);
      if (consecutive > 120) {
        HWCERROR(eCheckTooManyConsecutiveDroppedFrames,
                 "Display %d had %d consecutive dropped frames", i,
                 consecutive);
      }
    }
  }

  if (clear) {
    mStartFN = mFN;
  }
}

void HwcTestKernel::UpdateVideoState(int64_t sessionId, bool isPrepared) {
  HWCVAL_LOCK(_l, mMutex);
  ssize_t ix = mVideoSessions.indexOfKey(sessionId);

  if (isPrepared) {
    HWCLOGD_COND(eLogVideo, "HwcTestKernel::UpdateVideoState add session %d",
                 sessionId);
    if (ix < 0) {
      mVideoSessions.add(sessionId, 0);
    }
  } else {
    HWCLOGD_COND(eLogVideo, "HwcTestKernel::UpdateVideoState remove session %d",
                 sessionId);
    if (ix >= 0) {
      mVideoSessions.removeItemsAt(ix);
    }
  }
}

void HwcTestKernel::UpdateVideoStateLegacy(int sessionId, uint32_t state) {
#ifdef HWCVAL_TARGET_HAS_MULTIPLE_DISPLAY
  UpdateVideoState(sessionId,
                   ((state == android::intel::MDS_VIDEO_PREPARING) ||
                    (state == android::intel::MDS_VIDEO_PREPARED) ||
                    (state == android::intel::MDS_VIDEO_UNPREPARING)));
#else
  HWCVAL_UNUSED(sessionId);
  HWCVAL_UNUSED(state);
#endif
}

void HwcTestKernel::UpdateVideoFPS(int64_t sessionId, int32_t fps) {
  HWCVAL_LOCK(_l, mMutex);
  ssize_t ix = mVideoSessions.indexOfKey(sessionId);

  if (ix >= 0) {
    mVideoSessions.replaceValueAt(ix, fps);
  } else {
    HWCLOGW("UpdateVideoFPS called for invalid session %d", sessionId);
  }
}

// Call from harness to avoid the work queue getting too big
// Careful not to deadlock
void HwcTestKernel::ProcessWork() {
  HWCVAL_LOCK(_l, mMutex);
  ProcessWorkLocked();
}

void HwcTestKernel::ProcessWorkLocked() {
  mWorkQueue.Process();
}

HwcTestCrtc* HwcTestKernel::GetHwcTestCrtcByDisplayIx(uint32_t displayIx,
                                                      bool persistentCopy) {
  if (displayIx >= HWCVAL_MAX_CRTCS) {
    return 0;
  } else if (persistentCopy) {
    return mPersistentCrtcByDisplayIx[displayIx];
  } else {
    return mCrtcByDisplayIx[displayIx];
  }
}

void HwcTestKernel::StopThreads() {
  HWCLOGD("HwcTestKernel::StopThreads");
  if (mCompVal.get()) {
    mCompVal->KillThread();
  }

  for (uint32_t i = 0; i < HWCVAL_MAX_CRTCS; ++i) {
    if (mCrtcByDisplayIx[i]) {
      mCrtcByDisplayIx[i]->StopThreads();
    }
  }
}

void HwcTestKernel::SkipFrameValidation(HwcTestCrtc* crtc) {
  // Drop the frame from the LLQ
  uint32_t droppedFrames = 0;
  uint32_t displayIx = crtc->GetDisplayIx();

  if (displayIx < HWCVAL_MAX_CRTCS) {
    if (displayIx != HWCVAL_VD_DISPLAY_INDEX) {
      Hwcval::LayerList* ll = mLLQ[displayIx].GetFrame(mFN[displayIx], false);
      if (ll) {
        // TODO: other dropped frames
        // there was a final frame that should have been displayed. We call that
        // dropped too.
        ++droppedFrames;
      }
    }
    HWCLOGI("Final dropped frames: Display %d: %d", displayIx, droppedFrames);
    crtc->AddDroppedFrames(droppedFrames);
  }
}

void HwcTestKernel::FinaliseTest() {
  // Wait 3.1 seconds for stability - this is longer than the 3 second page flip
  // timeout, but still no guarantee.
  usleep(3100000);

  // Set eval counts where these couldn't be calculated at the time
  HWCCHECK_ADD(eCheckSfFallback, mPartitionedCompositionCount.GetValue() +
                                     mSfCompositionCount.GetValue());
  for (uint32_t i = 0; i < HWCVAL_MAX_CRTCS; ++i) {
    if (mCrtcByDisplayIx[i]) {
      // Finalise dropped frame counts
      SkipFrameValidation(mCrtcByDisplayIx[i]);
    }
  }

  // If there have been a lot of video layers restored during
  // supposed rotation animation then there must be something wrong

  // TODO: Something less hacky, or remove the check
  uint32_t frameCount = mFN[0] - mStartFN[0];
  if ((mSnapshotsRestored.GetValue() > 10) &&
      (mSnapshotsRestored.GetValue() > frameCount / 1000)) {
    HWCERROR(eCheckTooManySnapshotsRestored, "%d snaphots restored",
             mSnapshotsRestored.GetValue());
  }
}

void HwcTestKernel::CheckInvalidSessionsDisplayed() {
  char strbuf[HWCVAL_DEFAULT_STRLEN];
  char strbuf2[HWCVAL_DEFAULT_STRLEN];

  if ((systemTime(SYSTEM_TIME_MONOTONIC) - mLastOnPrepareTime) >
      2000000000)  // 2 seconds
  {
    // Invalidate call could have timed out
    // so it's not OK to do this check.
    return;
  }

  for (uint32_t i = 0; i < mPlanes.size(); ++i) {
    DrmShimPlane* plane = mPlanes.valueAt(i);
    ALOG_ASSERT(plane);
    HwcTestCrtc* crtc = plane->GetCrtc();

    if (crtc) {
      uint32_t displayIx = crtc->GetDisplayIx();

      if (displayIx == eNoDisplayIx) {
        HWCLOGW(
            "CheckInvalidSessionsDisplayed: Plane %d CRTC %p has no CRTC id",
            plane->GetPlaneId(), crtc);
        return;
      }

      ALOG_ASSERT(displayIx < HWCVAL_MAX_CRTCS);
      uint32_t hwcFrame = mFN[displayIx];

      android::sp<DrmShimBuffer> buf = plane->GetTransform().GetBuf();

    }
  }
}

void HwcTestKernel::MarkEsdRecoveryStart(uint32_t connectorId) {
  // Overriden in DrmShimChecks.
  HWCVAL_UNUSED(connectorId);
  ALOG_ASSERT(0);
}

void HwcTestKernel::EsdRecoveryReport() {
  for (uint32_t i = 0; i < HWCVAL_MAX_CRTCS; ++i) {
    HwcTestCrtc* crtc = mCrtcByDisplayIx[i];

    if (crtc) {
      crtc->EsdRecoveryEnd("did not complete in");
    }
  }
}

bool HwcTestKernel::IsTotalDisplayFail() {
  for (uint32_t i = 0; i < HWCVAL_MAX_CRTCS; ++i) {
    HwcTestCrtc* crtc = mCrtcByDisplayIx[i];

    if (crtc) {
      if (crtc->IsTotalDisplayFail()) {
        return true;
      }
    }
  }

  return false;
}

void HwcTestKernel::SetVideoRate(uint32_t disp, float videoRate) {
  HwcTestCrtc* crtc = mCrtcByDisplayIx[disp];
  if (crtc) {
    crtc->SetVideoRate(videoRate);
  }
}

uint32_t HwcTestKernel::GetMDSVideoRate() {
  uint32_t videoRate = 0;
  if (mVideoSessions.size() == 1) {
    videoRate = mVideoSessions.valueAt(0);
    HWCLOGV_COND(eLogVideo, "GetMDSVideoRate: 1 session, rate %d", videoRate);
  } else {
    // Can't validate the video rate if more than one is available.
    HWCLOGV_COND(eLogVideo, "GetMDSVideoRate: %d sessions, so no video rate",
                 mVideoSessions.size());
  }

  return videoRate;
}

void HwcTestKernel::MapGlobalId(int id, android::sp<DrmShimBuffer> buf) {
  ssize_t ix = mBuffersByGlobalId.indexOfKey(id);

  if (ix >= 0) {
    android::sp<DrmShimBuffer> oldBuf = mBuffersByGlobalId.valueAt(ix);

    if (oldBuf->GetGlobalId() == id) {
      oldBuf->SetGlobalId(0);
    }

    mBuffersByGlobalId.replaceValueAt(ix, buf);
  } else {
    mBuffersByGlobalId.add(id, buf);
  }

  buf->SetGlobalId(id);
}

uint32_t HwcTestKernel::CrtcIdToDisplayIx(uint32_t crtcId) {
  for (uint32_t d = 0; d < HWCVAL_MAX_CRTCS; ++d) {
    if (mCrtcByDisplayIx[d] == NULL) {
      continue;
    }
    if (mCrtcByDisplayIx[d]->GetCrtcId() == crtcId) {
      return d;
    }
  }

  return HWCVAL_MAX_CRTCS;
}

void HwcTestKernel::DoStall(Hwcval::StallType ix, Hwcval::Mutex* mtx) {
  mState->GetStall(ix).Do(mtx);
}

void HwcTestKernel::ValidateOptimizationMode(Hwcval::LayerList* ll) {
  if (!IsDDRFreqSupported()) {
    return;
  }

  bool expectLowDDR = false;
  bool expectNormalDDR = false;

  if (mForceLowDDRMode == mLastForceLowDDRMode) {
    expectLowDDR = mForceLowDDRMode;
    expectNormalDDR = !mForceLowDDRMode;
  }

  mLastForceLowDDRMode = mForceLowDDRMode;

  int autoLowDDR = 0;

  if (mChangingDDRMode) {
    // Can't validate, mode change is in progress, anything is possible
    return;
  }

  if (!expectLowDDR) {
    // Is the HWC option enabled to automatically switch this on?
    autoLowDDR = GetHwcOptionInt("autovideoddr");
    // TODO: cache this value at the time the option is set.

    if (autoLowDDR) {
      HWCLOGV_COND(
          eLogVideo,
          "ValidateOptimizationMode: autolowddr set: clearing expectNormalDDR");
      expectNormalDDR = false;

      if (!mState->HotPlugInProgress()) {
        uint32_t numDisplays = mActiveDisplays;

        // Currently low DDR mode is ONLY used when there is one display active
        // and it is the panel.
        // If the panel is disabled (extended mode) then low DDR should not be
        // used.
        // If this ceases to be the case, the second part of the following if
        // statement can be removed.
        if (numDisplays == 1 && mCrtcByDisplayIx[0] &&
            mCrtcByDisplayIx[0]->IsDisplayEnabled()) {
            expectLowDDR = (ll->GetVideoFlags().mFullScreenVideo == eTrue);
            expectNormalDDR = (ll->GetVideoFlags().mFullScreenVideo == eFalse);
            HWCLOGV_COND(eLogVideo,
                         "ValidateOptimizationMode: auto: 1 display FS %s "
                         "expectNormalDDR=%d expectLowDDR=%d",
                         TriStateStr(ll->GetVideoFlags().mFullScreenVideo),
                         expectNormalDDR, expectLowDDR);
        } else {
          HWCLOGV_COND(eLogVideo,
                       "ValidateOptimizationMode: auto: %d displays active, "
                       "not expecting low DDR",
                       numDisplays);
        }
      } else {
        HWCLOGV_COND(eLogVideo,
                     "ValidateOptimizationMode: Hotplug in progress, no DDR "
                     "expectation");
      }
    } else {
      HWCLOGV_COND(eLogVideo,
                   "ValidateOptimizationMode: autovideoddr disabled");
    }
  } else {
    HWCLOGV_COND(eLogVideo, "ValidateOptimizationMode: lowddr forced");
  }

  // Don't check if mode has only just changed
  if (mDDRMode == mDDRModeLastFrame) {
    if (expectLowDDR || expectNormalDDR) {
      HWCCHECK(eCheckDDRMode);
    }

    if (expectLowDDR) {
      if (mDDRModeLastFrame == 0) {
        HWCERROR(eCheckDDRMode,
                 "DDR mode is normal, we are expecting LOW. (force %d auto %d)",
                 mForceLowDDRMode, autoLowDDR);
      } else {
        HWCLOGV_COND(eLogVideo, "Low DDR mode selected and validated");
      }
    }

    if (expectNormalDDR) {
      if (mDDRModeLastFrame) {
        HWCERROR(eCheckDDRMode, "DDR mode is LOW, we are expecting NORMAL.");
      }
    }
  }

  mDDRModeLastFrame = mDDRMode;
}

bool HwcTestKernel::IsWidiVideoMode(Hwcval::LayerList::VideoFlags vf,
                                    uint32_t hwcFrame) {
  if (!mState->IsAutoExtMode()) {
    if (mVideoSessions.size() == 0) {
      return false;
    }
  }

  return !mExtVideoModeDisabled &&
         (vf.mSingleFullScreenVideo || vf.mPartScreenVideo ||

          // Rotation animation still in progress so we won't leave extended
          // mode
          (hwcFrame <= mRotationEndFrame));
}

void HwcTestKernel::CompleteWidiDisable() {
  if (mWidiState == eWidiDisabling) {
    mWidiState = eWidiDisabled;
  }
}

// Record the "snapshot" buffer used in rotation animation.
void HwcTestKernel::SetSnapshot(HWCNativeHandle handle, uint32_t keepCount) {
  HWCVAL_UNUSED(keepCount);

  // Global snapshot expiry
  if (mRotationEndFrame + 1 < mFN[0]) {
    // New rotation
    mRotationStartFrame = mFN[0];
    mRotationEndFrame = mFN[0];
  } else {
    // Existing rotation being extended
    mRotationEndFrame = mFN[0];
  }

  ssize_t ix = mSnapshots.indexOfKey(handle);

  HWCLOGD("SetSnapshot: Buffer handle %p is snapshot. Rotation frame:%d-%d",
          handle, mRotationStartFrame, mRotationEndFrame);
  if (ix >= 0) {
    uint32_t& expiryFrame = mSnapshots.editValueAt(ix);
    expiryFrame = mRotationEndFrame;
  } else {
    mSnapshots.add(handle, mRotationEndFrame);
  }
}

bool HwcTestKernel::IsRotationInProgress(uint32_t hwcFrame) {
  bool ret =
      ((hwcFrame >= mRotationStartFrame) && (hwcFrame <= mRotationEndFrame));

  return ret;
}

// Is this buffer the snapshot for rotation animation?
bool HwcTestKernel::IsSnapshot(HWCNativeHandle handle, uint32_t hwcFrame) {
  if (mSnapshots.size() > 0) {
    ssize_t ix = mSnapshots.indexOfKey(handle);
    if (ix >= 0) {
      uint32_t expiryFrame = mSnapshots.valueAt(ix);

      if (hwcFrame <= expiryFrame) {
        HWCLOGD(
            "IsSnapshot: Buffer handle %p is snapshot now and until frame %d",
            handle, expiryFrame);
        return true;
      } else {
        HWCLOGD(
            "IsSnapshot: Deleting snapshot record for handle %p, now frame:%d, "
            "expired at %d",
            handle, hwcFrame, expiryFrame);
        mSnapshots.removeItemsAt(ix);
      }
    }
  }

  return false;
}
