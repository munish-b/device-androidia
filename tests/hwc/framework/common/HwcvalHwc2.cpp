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

#include "HwcTestState.h"
#include "HwcvalHwc2.h"
#include "HwcvalHwc1Content.h"
#include "HwcvalThreadTable.h"
#include "HwcvalStall.h"

#undef LOG_TAG
#define LOG_TAG "DRM_SHIM"

using namespace Hwcval;

/// Constructor
Hwcval::Hwc2::Hwc2()
    : mState(HwcTestState::getInstance()), mHwcFrame((uint32_t)-1) {
  mTestKernel = mState->GetTestKernel();
}

EXPORT_API void Hwcval::Hwc2::CheckValidateDisplayEntry() {
  // TODO Adding numDisplays as local var, enumerate correctly in future
  uint32_t numDisplays = 3;
  // HWC frame number
  ++mHwcFrame;
  bool divergeFrameNumbers = mState->IsOptionEnabled(eOptDivergeFrameNumbers);

  for (uint32_t displayIx = 0; displayIx < numDisplays; ++displayIx) {
    if (!divergeFrameNumbers) {
      mTestKernel->AdvanceFrame(displayIx, mHwcFrame);
    }
    if (divergeFrameNumbers) {
      mTestKernel->AdvanceFrame(displayIx);
    }
  }
}

EXPORT_API void Hwcval::Hwc2::CheckValidateDisplayExit() {
  HWCLOGD("In CheckOnPrepareExit");
}

/// Called by HWC Shim to notify that Present Display has occurred, and pass
/// in
/// the
/// contents of the display structures
EXPORT_API void Hwcval::Hwc2::CheckPresentDisplayEnter(hwcval_display_contents_t* displays, hwc2_display_t display) {
// Dump memory usage, if enabled
  DumpMemoryUsage();
  int numDisplays = 1;
  ALOGE("%p layers = %d",displays,displays->numHwLayers);
  Hwcval::PushThreadState ts("CheckSetEnter (locking)");
  HWCVAL_LOCK(_l, mTestKernel->GetMutex());
  Hwcval::SetThreadState("CheckSetEnter (locked)");

  // Process any pending events.
  // This should always be the first thing we do after placing the lock.
  mTestKernel->ProcessWorkLocked();

  // Create a copy of the layer lists for each display in internal form.
  // Later on we will push this to the LLQ.
  memset(mContent, 0, sizeof(mContent));
  for (uint32_t d = 0; d < numDisplays; ++d) {
    const hwcval_display_contents_t* disp = displays;

    if (disp) {
      Hwcval::LayerList* ll = new Hwcval::Hwc1LayerList(disp);
      mContent[d] = ll;
    } else {
      mContent[d] = 0;
    }
  }

  // The idea of the buffer monitor enable was to be able to turn off the
  // majority of the validation for specific performance tests.
  // This has not been used in anger and would not work.
  // TODO: Fix or remove buffer monitor enable.
  if (mState->IsBufferMonitorEnabled()) {
    if (numDisplays == 0) {
      return;
    }

    // This loop has the following purposes:
    // 1. Record the state of each of the input buffers. That means that we
    // create a DrmShimBuffer object
    //    and track it by our internal data structures. These data structures
    //    are then augmented by later information
    //    from intercepted DRM calls that will allow us to understand the
    //    relationships between gralloc buffer handle,
    //    buffer object and frambuffer ID.
    //
    // 2. If any buffers are to be surface flinger composed - i.e. they have a
    // composition type of HWC2_COMPOSITION_CLIENT -
    //    then a transform mapping is created to track this surface flinger
    //    composition. This is then attached to
    //    the DrmShimBuffer of the framebuffer target.
    //
    // 3. Determining for each display whether there is full screen video.
    //    These are then combined to create the flags that are needed for
    //    extended mode validation.
    //    They is then saved within the VideoFlags of the internal layer list.
    //
    // 4. Recording of protected content validity. To avoid spurious errors it
    // is important that this is recorded
    //    at the right time, so we are actually caching in the layer list a
    //    state that was recorded during onValidity.
    //
    // 5. Some additional flag setting and statistic recording.
    //
    bool allScreenVideo = true;  // assumption that all screens have video on
                                 // top layer until we know otherwise

    for (uint32_t displayIx = 0; displayIx < numDisplays; ++displayIx) {
      Hwcval::LogDisplay& ld = mTestKernel->GetLogDisplay(displayIx);
      const hwcval_display_contents_t* disp = displays;
      mTestKernel->VideoInit(displayIx);

      if (disp == 0) {
        if (displayIx == 0) {
          HWCLOGW("Set has null display[0]");
        }

        mTestKernel->SetActiveDisplay(displayIx, false);

        // Ignore null HDMI or WiDi displays
        continue;
      }

      mTestKernel->SetActiveDisplay(displayIx, true);
  ALOGE("%p layers = %d",disp,disp->numHwLayers);

      HWCLOGD(
          "HwcTestKernel::CheckSetEnter - Display %d has %u layers (frame:%d)",
          displayIx, disp->numHwLayers, mHwcFrame);

      DrmShimTransformVector framebuffersComposedForThisTarget;
      bool sfCompositionRequired = false;
      uint32_t skipLayerCount = 0;

      if (disp->numHwLayers == 0) {
        // No content on this screen, so definitely no video
        HWCLOGV_COND(eLogVideo,
                     "No content on screen %d, so definitely no video",
                     displayIx);
        allScreenVideo = false;
      }

      const hwcval_layer_t* fbtLayer = disp->hwLayers + disp->numHwLayers - 1;
      ALOG_ASSERT(fbtLayer->compositionType == HWC2_COMPOSITION_DEVICE);

      const hwc_rect_t& fbtRect = fbtLayer->displayFrame;
      if (displayIx < numDisplays) {
        if (fbtRect.right != ld.GetWidth() ||
            fbtRect.bottom != ld.GetHeight() || fbtRect.left != 0 ||
            fbtRect.top != 0) {
          HWCERROR(eCheckLayerOnScreen,
                   "D%d FBT (%d, %d, %d, %d) but display size %dx%d", displayIx,
                   fbtRect.left, fbtRect.top, fbtRect.right, fbtRect.bottom,
                   ld.GetWidth(), ld.GetHeight());
        }
      }

      for (uint32_t i = 0; i < disp->numHwLayers; ++i) {
        const hwcval_layer_t* layer = disp->hwLayers + i;
        const char* bufferType = "Unknown";
        android::sp<DrmShimBuffer> buf;
        char notes[HWCVAL_DEFAULT_STRLEN];
        notes[0] = '\0';

        switch (layer->compositionType) {
          case HWC2_COMPOSITION_CLIENT: {
            sfCompositionRequired = true;
            bufferType = "Framebuffer";

            if (layer->flags & HWC_SKIP_LAYER) {
              ++skipLayerCount;
            }

            if (layer->gralloc_handle == 0) {
              buf = 0;
            } else {
              buf = mTestKernel->RecordBufferState(
                  layer->gralloc_handle, Hwcval::BufferSourceType::Input, notes);

              if ((layer->flags & HWC_SKIP_LAYER) == 0) {
                mTestKernel->ValidateHwcDisplayFrame(layer->displayFrame,
                                                     fbtRect, displayIx, i);
                DrmShimTransform transform(buf, i, layer);
                framebuffersComposedForThisTarget.add(transform);

                mTestKernel->AddSfScaleStat(transform.GetXScale());
                mTestKernel->AddSfScaleStat(transform.GetYScale());
              }
            }

            break;
          }
          case HWC2_COMPOSITION_DEVICE: {
            if (layer->gralloc_handle == 0) {
              buf = 0;
            } else if ((layer->flags & HWC_SKIP_LAYER) == 0) {
              bufferType = "Overlay";
              mTestKernel->ValidateHwcDisplayFrame(layer->displayFrame, fbtRect,
                                                   displayIx, i);
              buf = mTestKernel->RecordBufferState(
                  layer->gralloc_handle, Hwcval::BufferSourceType::Input, notes);
            } else {
              bufferType = "Overlay (SKIP)";
              buf = mTestKernel->RecordBufferState(
                  layer->gralloc_handle, Hwcval::BufferSourceType::Input, notes);
              DrmShimTransform transform(buf, i, layer);
              framebuffersComposedForThisTarget.add(transform);
            }

            break;
          }
        }

        if (buf.get() &&
            buf->GetHandle() == mState->GetFutureTransparentLayer()) {
          HWCLOGW("Actually transparent: %p AppearanceCount %d",
                  buf->GetHandle(), buf->GetAppearanceCount());
          buf->SetTransparentFromHarness();
        }
        ALOGE("%d", layer->visibleRegionScreen.numRects);

        Hwcval::Hwc1Layer valLayer(layer, buf);
        // Work out if we are full screen video on each display
        mTestKernel->DetermineFullScreenVideo(displayIx, i, valLayer, notes);

        // Skip layers will be subject to the skip layer usage check
        HWCCHECK_ADD(eCheckSkipLayerUsage, skipLayerCount);

        // Are we skipping all layers? That means rotation animation
        HwcTestCrtc* crtc = mTestKernel->GetHwcTestCrtcByDisplayIx(i);
        if (crtc) {
          crtc->SetSkipAllLayers((disp->numHwLayers > 1) &&
                                 (skipLayerCount == (disp->numHwLayers - 1)));
        }

        // Add the layer to our internal layer list copy
        mContent[displayIx]->Add(valLayer);
      }
    }

    // Work out combined video state flags, by looking at the current state of
    // all displays
    Hwcval::LayerList::VideoFlags videoFlags = mTestKernel->AnalyzeVideo();

    // Set the combined video state flags on each of the current displays before
    // we push them.
    // (Question: does this leave us in a mess if a display is not updated? Does
    // that mean
    // it could end up with us thinking it is in the wrong mode?)
    for (uint32_t d = 0; d < HWCVAL_MAX_CRTCS; ++d) {
      Hwcval::LayerList* ll = mContent[d];

      if (ll) {
        HWCLOGV_COND(eLogVideo,
                     "Frame:%d: Content@%p: Setting video flags for D%d",
                     mHwcFrame, ll, d);
        ll->SetVideoFlags(videoFlags);
        ll->GetVideoFlags().Log("CheckSetEnter", d, mHwcFrame);

        HWCLOGV_COND(eLogWidi, "Pushing SF%d frame:%d %s", d, mHwcFrame,
                     (d == 2) ? "virtual display" : "");
        mTestKernel->GetLLQ(d)
            .Push(ll, mHwcFrame);  // FIX: frame number needs to be per-display
      }
    }

    if (mState->IsCheckEnabled(eCheckSfCompMatchesRef)) {
      // Validate surface flinger composition against reference composer
      for (uint32_t d = 0; d < HWCVAL_MAX_CRTCS; ++d) {
        Hwcval::LayerList* ll = mContent[d];

        if (ll && (ll->GetNumLayers() > 0)) {
          uint32_t fbTgtLayerIx = ll->GetNumLayers() - 1;
          Hwcval::ValLayer& fbTgtLayer = ll->GetLayer(fbTgtLayerIx);

          android::sp<DrmShimBuffer> fbTgtBuf = fbTgtLayer.GetBuf();

          if (fbTgtBuf.get()) {
            Hwcval::LayerList srcLayers;
            for (uint32_t i = 0; i < fbTgtLayerIx; ++i) {
              Hwcval::ValLayer& layer = ll->GetLayer(i);
              if (layer.GetCompositionType() == CompositionType::SF) {
                srcLayers.Add(layer);
              }
            }

            if (srcLayers.GetNumLayers() > 0) {
              HWCLOGD("Sf Comp Val: Starting for handle %p",
                      fbTgtBuf->GetHandle());
              mTestKernel->GetCompVal()->Compose(fbTgtBuf, srcLayers,
                                                 fbTgtLayer);
              mTestKernel->GetCompVal()->Compare(fbTgtBuf);
            } else {
              HWCLOGD("Sf Comp Val: No layers for handle %p",
                      fbTgtBuf->GetHandle());
            }
          }
        }
      }
    }
  }
}

void Hwcval::Hwc2::CheckPresentDisplayExit(hwcval_display_contents_t* displays, hwc2_display_t display, int32_t *outPresentFence) {
  HWCLOGI("CheckSetExit frame:%d", mHwcFrame);
  int numDisplays = 1;
  PushThreadState ts("CheckSetExit");

  // Clear the future transparent layer notification from the harness
  mState->SetFutureTransparentLayer(0);

  mActiveDisplays = 0;
  // Count the number of active displays
  // We may need to add a flag so users of this variable know if it has changed
  // recently
  // so they don't validate too harshly.
  for (uint32_t d = 0; d < numDisplays; ++d) {
    HwcTestCrtc* crtc = mTestKernel->GetHwcTestCrtcByDisplayIx(d);
    hwcval_display_contents_t* disp = displays + d;

    if (crtc && disp && crtc->IsDisplayEnabled()) {
      ++mActiveDisplays;
    }
  }

  mTestKernel->SetActiveDisplays(mActiveDisplays);

  if ((numDisplays > 0) && displays) {
    int retireFence[HWCVAL_MAX_CRTCS];

    for (uint32_t d = 0; d < numDisplays; ++d) {
      retireFence[d] = -1;
    }

    // Sort the retire fences back to the original displays. This is because the
    // HWC
    // will move the retire fence index from the secondary display to D0 in
    // extended mode.
    for (uint32_t d = 0; d < numDisplays; ++d) {
      hwcval_display_contents_t* disp = displays + d;

      // If no display, continue
      if (!disp) {
        continue;
      }
      ALOGE("*outPresentFence%d ",*outPresentFence);
      int rf = *outPresentFence;

      // There should be a retire fence on the primary display - check.
      if (d == 0) {
       // HWCCHECK(eCheckNoRetireFenceOnPrimary);
        if (rf < 0) {
         // HWCERROR(eCheckNoRetireFenceOnPrimary,
           //        "D0 has retire fence=%d frame:%d", rf, mHwcFrame);
        }
      }

      // If not found retire fence
      if (rf <= 0) {
       // continue;
      }

      // Assign the fence, no longer worry about original display.
      retireFence[d] = rf;
    }

    for (uint32_t d = 0; d < numDisplays; ++d) {
      hwcval_display_contents_t* disp = displays + d;
      Hwcval::LayerListQueue& llq = mTestKernel->GetLLQ(d);

      if (disp && llq.BackNeedsValidating()) {
        Hwcval::LayerList* ll = llq.GetBack();
        int hwcFence = retireFence[d];
        HWCCHECK(eCheckFenceNonZero);
        if (hwcFence == 0) {
          HWCERROR(eCheckFenceNonZero,
                   "Zero retire fence detected on display %d", d);
        }

        if (hwcFence >= 0) {
          int rf = dup(hwcFence);

          // We were having trouble with zero fences.
          // This turned out to be because, owing to another bug of our own and
          // a lack of checking in HWC,
          // HWC was closing FD 0.
          //
          // This code makes us more tolerant of FD 0 if it arises (but it is
          // definitely a bad thing).
          HWCCHECK(eCheckFenceNonZero);
          HWCCHECK(eCheckFenceAllocation);
          if (rf <= 0) {
            HWCERROR((rf == 0) ? eCheckFenceNonZero : eCheckFenceAllocation,
                     "Fence dup failed. Display %d: Dup of %d was %d.", d,
                     hwcFence, rf);

            HWCLOGD("Display %d: Dup of %d was %d, trying again", d, hwcFence,
                    rf);
            rf = dup(hwcFence);
          }

          if (rf < 0) {
            HWCLOGD("Display %d: Failed to dup retire fence %d", d, hwcFence);
          }

          ll->SetRetireFence(rf);
          HwcTestCrtc* crtc = mTestKernel->GetHwcTestCrtcByDisplayIx(d);

          if (crtc == 0) {
            HWCLOGW("CheckSetExit: Display %d: No CRTC known", d);
          } else {
            crtc->NotifyRetireFence(rf);
          }
        } else {
          ll->SetRetireFence(-1);
        }

        // HWCLOGI_COND(eLogFence, "  -- Display %d retire fence %d dup
        // %d", d, disp->retireFenceFd, ll->GetRetireFence());
      }
    }

    if ((mTestKernel->GetHwcTestCrtcByDisplayIx(0, true) == 0) &&
        (mHwcFrame == 100)) {
      HWCERROR(eCheckInternalError, "No D0 defined within first 100 frames.");
    }
  }

  if (mState->IsBufferMonitorEnabled()) {
    if ((numDisplays > 2) && (mTestKernel->IsWidiDisabled())) {
      // Check frames on Virtual displays.
      //
      // Note that
      // physical displays (e.g. the panel and HDMI) are validated in the
      // page flip handler so we only need to check Virtual display frames
      // here (Widi frames are validated in the Widi shim).
      HWCLOGD_COND(eLogDrm, "Asynch DRM, numDisplays=%d, checks on display %d",
                   numDisplays, eDisplayIxVirtual);

      HwcTestCrtc* crtc =
          mTestKernel->GetHwcTestCrtcByDisplayIx(eDisplayIxVirtual);
      PushThreadState ts("CheckSetExit (locking)");
      HWCVAL_LOCK(_l, mTestKernel->GetMutex());
      SetThreadState("CheckSetExit (locked)");

      // Only validate each widi frame once
      Hwcval::LayerListQueue& llq = mTestKernel->GetLLQ(eDisplayIxVirtual);
      Hwcval::LayerList* ll = llq.GetBack();

      if (crtc && ll && ll->GetOutbuf()) {
        uint32_t qFrame = llq.GetFrontFN();
        if (qFrame <= mHwcFrame) {
          HWCLOGD_COND(eLogWidi, "Checking Virtual Display frame:%d",
                       mHwcFrame);
          uint32_t startFrame = llq.GetFrontFN();
          if (startFrame < mHwcFrame) {
            HWCLOGD_COND(eLogWidi, "Discarding virtual display frames:%d-%d",
                         startFrame, mHwcFrame - 1);
          }

          // Pop all virtual display frames up to the current one
          for (qFrame = llq.GetFrontFN(); qFrame <= mHwcFrame; ++qFrame) {
            ll = llq.GetFrame(qFrame, true);
          }
        }

        qFrame = llq.GetFrontFN();

        if (qFrame == mHwcFrame) {
        } else {
          HWCLOGD_COND(eLogWidi,
                       "Skipping virtual display validation. Last HWC frame:%d "
                       "current frame:%d",
                       llq.GetBackFN(), mHwcFrame);
        }
      } else {
        HWCLOGD_COND(eLogWidi,
                     "SF2 frame:%d is NOT virtual display (no outbuf)",
                     mHwcFrame);
      }
    }
  }

  // Optimization mode is decided in onPrepare so it is correct to do this here
  // rather than on page flip event.
  Hwcval::LayerList* ll = mTestKernel->GetLLQ(eDisplayIxFixed).GetBack();
  if (ll) {
    HWCLOGV_COND(eLogVideo,
                 "Frame:%d Validating optimization mode for D%d (content@%p)",
                 mHwcFrame, eDisplayIxFixed, ll);
    ll->GetVideoFlags().Log("CheckSetExit", eDisplayIxFixed, mHwcFrame);
    mTestKernel->ValidateOptimizationMode(ll);
  }

  // This works best here, because it avoids causing errors from display
  // blanking
  // at the start of the next frame.
  {
    PushThreadState ts("CheckSetExit (locking)");
    HWCVAL_LOCK(_l, mTestKernel->GetMutex());
    SetThreadState("CheckSetExit (locked)");
    mTestKernel->ProcessWorkLocked();

    mTestKernel->IterateAllBuffers();
  }

  // Advance Widi state if disconnecting
  mTestKernel->CompleteWidiDisable();

  // Dump memory usage, if enabled
  DumpMemoryUsage();

}

void Hwcval::Hwc2::GetDisplayConfigsExit(int disp, uint32_t* configs,
                                         uint32_t numConfigs) {
  if (disp < HWCVAL_MAX_LOG_DISPLAYS) {
    mTestKernel->GetLogDisplay(disp).SetConfigs(configs, numConfigs);
  } else {
    HWCERROR(eCheckHwcParams, "getDisplayConfigs D%d", disp);
  }
}


void Hwcval::Hwc2::GetDisplayAttributesExit(uint32_t disp, uint32_t config, const int32_t attribute, int32_t* values)
{
    if (disp < HWCVAL_MAX_LOG_DISPLAYS)
    {
        mTestKernel->GetLogDisplay(disp).SetDisplayAttributes(config, attribute, values);
    }
    else
    {
        HWCERROR(eCheckHwcParams, "getDisplayAttributes D%d config %d", disp, config);
    }
}
