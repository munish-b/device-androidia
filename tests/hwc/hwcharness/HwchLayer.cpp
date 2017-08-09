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

#include "HwchLayer.h"
#include "HwchFrame.h"
#include "HwchSystem.h"
#include "HwchDisplay.h"
#include "HwchTimelineThread.h"
#include "HwchDefs.h"
#include "HwcTestLog.h"
#include "HwcTestState.h"
#include "HwcTestUtil.h"
#include <ui/GraphicBuffer.h>
#include <graphics.h>
#include <DrmShimBuffer.h>

Hwch::Rect::Rect(uint32_t l, uint32_t t, uint32_t r, uint32_t b) {
  left = l;
  top = t;
  right = r;
  bottom = b;
}

void Hwch::Rect::Rotate(RotationType rot, Display& display) {
  uint32_t l = left;
  uint32_t t = top;
  uint32_t r = right;
  uint32_t b = bottom;

  switch (rot) {
    case eRotate90: {
      left = display.GetLogicalWidth() - b;
      top = l;
      right = display.GetLogicalWidth() - t;
      bottom = r;
      break;
    }

    case eRotate180: {
      left = display.GetLogicalWidth() - r;
      right = display.GetLogicalWidth() - l;
      top = display.GetLogicalHeight() - b;
      bottom = display.GetLogicalHeight() - t;
      break;
    }

    case eRotate270: {
      left = t;
      top = display.GetLogicalHeight() - r;
      right = b;
      bottom = display.GetLogicalHeight() - l;
      break;
    }

    default: {}
  }
}

const char* Hwch::Rect::Str() {
  static char buf[100];
  sprintf(buf, "(%d, %d, %d, %d)", left, top, right, bottom);
  return buf;
}

Hwch::FRect::FRect(float l, float t, float r, float b) {
  left = l;
  top = t;
  right = r;
  bottom = b;
}

const char* Hwch::FRect::Str() {
  static char buf[100];
  sprintf(buf, "(%f, %f, %f, %f)", (double)left, (double)top, (double)right,
          (double)bottom);
  return buf;
}

Hwch::FRect::FRect(double l, double t, double r, double b) {
  left = l;
  top = t;
  right = r;
  bottom = b;
}

Hwch::FRect::FRect(uint32_t l, uint32_t t, uint32_t r, uint32_t b) {
  left = float(l);
  top = float(t);
  right = float(r);
  bottom = float(b);
}

static uint32_t sLayerCount = 0;

static void IncLayerCount() {
  if (++sLayerCount > 100) {
    if (sLayerCount > 500) {
      HWCERROR(eCheckInternalError, "Layer leak: %d layers allocated",
               sLayerCount);
    } else {
      HWCLOGD_COND(eLogLayerAlloc, "Currently %d layers allocated (+)",
                   sLayerCount);
    }
  }
}

static void DecLayerCount() {
  --sLayerCount;
}

Hwch::Layer::Layer()
    : mCompType(0),
      mHints(0),
      mFlags(0),
      mLogicalTransform(0),
      mPhysicalTransform(0),
      mBlending(HWC_BLENDING_PREMULT),
      mTile(eAnyTile),
      mEncrypted(eNotEncrypted),
      mIgnoreScreenRotation(false),
      mHwcAcquireDelay(0),
      mPattern(0),
      mSourceCropf(0.0, 0.0, 0.0, 0.0),
      mDisplayFrame(0, 0, 0, 0),
      mGeometryChanged(false),
      mIsForCloning(false),
      mUpdatedSinceFBComp(HWCH_ALL_DISPLAYS_UPDATED),
      mIsACloneOf(0),
      mFrame(0),
      mName("Undefined"),
      mSystem(Hwch::System::getInstance()) {
  memset(mClonedLayers, 0, sizeof(mClonedLayers));
  IncLayerCount();
}

Hwch::Layer::Layer(const char* name, Hwch::Coord<int32_t> width,
                   Hwch::Coord<int32_t> height, uint32_t pixelFormat,
                   int32_t numBuffers, uint32_t usage)
    : mCompType(HWC2_COMPOSITION_CLIENT),
      mCurrentCompType(HWC2_COMPOSITION_CLIENT),
      mHints(0),
      mFlags(0),
      mLogicalTransform(0),
      mPhysicalTransform(0),
      mBlending(HWC_BLENDING_PREMULT),
      mFormat(pixelFormat),
      mPlaneAlpha(255),
      mWidth(width),
      mHeight(height),
      mUsage(usage),
      mTile(eAnyTile),
      mEncrypted(eNotEncrypted),
      mIgnoreScreenRotation(false),
      mHwcAcquireDelay(0),
      mNeedBuffer(true),
      mPattern(0),
      mLogicalCropf(0.0, 0.0, CoordUnassigned<float>(),
                    CoordUnassigned<float>()),
      mSourceCropf(0.0, 0.0, 0.0, 0.0),
      mLogicalDisplayFrame(0, 0, width, height),
      mDisplayFrame(0, 0, 0, 0),
      mGeometryChanged(true),
      mIsForCloning(false),
      mUpdatedSinceFBComp(HWCH_ALL_DISPLAYS_UPDATED),
      mIsACloneOf(0),
      mFrame(0),
      mName(name),
      mSystem(Hwch::System::getInstance()) {
  if (numBuffers < 0) {
    mNumBuffers = Hwch::System::getInstance().GetDefaultNumBuffers();
  } else {
    mNumBuffers = numBuffers;
  }

  HWCLOGI("Constructing layer %s %dx%d pixelFormat=%d numBuffers=%d usage=0x%x",
          mName.string(), mWidth.mValue, mHeight.mValue, mFormat, numBuffers,
          usage);
  memset(mClonedLayers, 0, sizeof(mClonedLayers));

  IncLayerCount();
}

Hwch::Layer::Layer(const Layer& rhs, bool clone)
    : mCompType(HWC2_COMPOSITION_CLIENT),
      mCurrentCompType(HWC2_COMPOSITION_CLIENT),
      mHints(rhs.mHints),
      mFlags(rhs.mFlags),
      mLogicalTransform(rhs.mLogicalTransform),
      mPhysicalTransform(0),
      mBlending(rhs.mBlending),
      mFormat(rhs.mFormat),
      mNumBuffers(rhs.mNumBuffers),
      mPlaneAlpha(rhs.mPlaneAlpha),
      mWidth(rhs.mWidth),
      mHeight(rhs.mHeight),
      mUsage(rhs.mUsage),
      mTile(rhs.mTile),
      mEncrypted(rhs.mEncrypted),
      mIgnoreScreenRotation(rhs.mIgnoreScreenRotation),
      mHwcAcquireDelay(rhs.mHwcAcquireDelay),
      mNeedBuffer(rhs.mNeedBuffer),
      mPattern(rhs.mPattern),
      mBufs(rhs.mBufs),
      mLogicalCropf(rhs.mLogicalCropf),
      mSourceCropf(rhs.mSourceCropf),
      mLogicalDisplayFrame(rhs.mLogicalDisplayFrame),
      mDisplayFrame(rhs.mDisplayFrame),
      mVisibleRegion(rhs.mVisibleRegion),
      mGeometryChanged(true),
      mIsForCloning(false),
      mUpdatedSinceFBComp(clone ? HWCH_ALL_DISPLAYS_UPDATED
                                : rhs.mUpdatedSinceFBComp),
      mIsACloneOf(clone ? const_cast<Layer*>(&rhs) : rhs.mIsACloneOf),
      mFrame(0),
      mName(rhs.mName),
      mSystem(Hwch::System::getInstance())

{
  memset(mClonedLayers, 0, sizeof(mClonedLayers));
  IncLayerCount();
}

Hwch::Layer::~Layer() {
  // Delete any layers to which we have cloned this layer
  for (uint32_t i = 0; i < MAX_DISPLAYS; ++i) {
    if (mClonedLayers[i]) {
      HWCLOGD_COND(eLogLayerAlloc,
                   "Layer@%p::~Layer() %s Deleting cloned layer D%d @ %p", this,
                   GetName(), i, mClonedLayers[i]);
      delete mClonedLayers[i];
      mClonedLayers[i] = 0;
    }
  }

  // Remove yourself from the frame.
  if (mFrame) {
    HWCLOGV("Removing layer %s@%p from frame", GetName(), this);
    mFrame->Remove(*this);
  }
  DecLayerCount();
  HWCLOGV_COND(eLogLayerAlloc, "Layer@%p::~Layer() exit", this);
}

Hwch::Layer& Hwch::Layer::operator=(const Hwch::Layer& rhs) {
  mCompType = rhs.mCompType;
  mCurrentCompType = rhs.mCurrentCompType;
  mHints = rhs.mHints;
  mFlags = rhs.mFlags;
  mLogicalTransform = rhs.mLogicalTransform;
  mBlending = rhs.mBlending;
  mFormat = rhs.mFormat;
  mPlaneAlpha = rhs.mPlaneAlpha;
  mWidth = rhs.mWidth;
  mHeight = rhs.mHeight;
  mUsage = rhs.mUsage;
  mEncrypted = rhs.mEncrypted;
  mIgnoreScreenRotation = rhs.mIgnoreScreenRotation;
  mHwcAcquireDelay = rhs.mHwcAcquireDelay;
  mNeedBuffer = rhs.mNeedBuffer;
  mPattern = rhs.mPattern;
  mBufs = rhs.mBufs;
  mSourceCropf = rhs.mSourceCropf;
  mDisplayFrame = rhs.mDisplayFrame;
  mVisibleRegion = rhs.mVisibleRegion;
  mName = rhs.mName;

  HWCLOGI("Layer @ %p: Assignment: &mSystem=%p", this, &mSystem);
  return *this;
}

Hwch::Layer* Hwch::Layer::Dup() {
  return new Layer(*this);
}

void Hwch::Layer::SetCompositionType(uint32_t compType) {
  mCompType = compType;
  mCurrentCompType = compType;
}

void Hwch::Layer::SetCrop(const Hwch::LogCropRect& rect) {
  if (rect != mLogicalCropf) {
    mLogicalCropf = rect;
    SetUpdated();
  }
}

void Hwch::Layer::SetIsACloneOf(Hwch::Layer* clone) {
  mIsACloneOf = clone;
}

const Hwch::LogCropRect& Hwch::Layer::GetCrop() {
  return mLogicalCropf;
}

void Hwch::Layer::SetLogicalDisplayFrame(const LogDisplayRect& rect) {
  if (rect != mLogicalDisplayFrame) {
    mLogicalDisplayFrame = rect;
    SetUpdated();
  }
}

const Hwch::LogDisplayRect& Hwch::Layer::GetLogicalDisplayFrame() {
  return mLogicalDisplayFrame;
}

void Hwch::Layer::SetOffset(const Coord<int>& x, const Coord<int>& y) {
  LogDisplayRect newRect;
  newRect.left = x;
  newRect.top = y;
  newRect.right = CoordUnassigned<int32_t>();
  newRect.bottom = CoordUnassigned<int32_t>();

  SetLogicalDisplayFrame(newRect);
}

void Hwch::Layer::SetBlending(uint32_t blending) {
  if (blending != mBlending) {
    mBlending = blending;
    SetUpdated();
  }
}

void Hwch::Layer::SetTransform(uint32_t transform) {
  if (transform != mLogicalTransform) {
    mLogicalTransform = transform;
    SetUpdated();
  }
}

void Hwch::Layer::SetPlaneAlpha(uint32_t planeAlpha) {
  if (planeAlpha != mPlaneAlpha) {
    mPlaneAlpha = planeAlpha;
    SetUpdated();
  }
}

void Hwch::Layer::SetIgnoreScreenRotation(bool ignore) {
  mIgnoreScreenRotation = ignore;
}

void Hwch::Layer::SetHwcAcquireDelay(uint32_t delay) {
  mHwcAcquireDelay = delay;
}

void Hwch::Layer::SetPattern(Pattern* pattern) {
  mPattern = pattern;
  pattern->Init();
}

Hwch::Pattern& Hwch::Layer::GetPattern() {
  return *(mPattern.get());
}

uint32_t Hwch::Layer::GetFlags() {
  return mFlags;
}

void Hwch::Layer::SetFlags(uint32_t flags) {
  mFlags = flags;
}

void Hwch::Layer::SetSkip(bool skip, bool needBuffer) {
  uint32_t flags = mFlags;

  if (skip) {
    mFlags |= HWC_SKIP_LAYER;
  } else {
    mFlags &= ~HWC_SKIP_LAYER;
  }

  if (flags != mFlags) {
    mGeometryChanged = true;
  }

  mNeedBuffer = needBuffer;

  if (!needBuffer) {
    mBufs = 0;
  }
}
#if 0
void Hwch::Layer::AssignLayerProperties(hwc2_layer_t& hwLayer, buffer_handle_t handle)
{
    hwLayer.handle          = handle;
    hwLayer.compositionType = mCurrentCompType;
    hwLayer.hints           = mHints;

    hwLayer.flags           = GetFlags();
    hwLayer.transform       = mPhysicalTransform;
    hwLayer.blending        = mBlending;

    hwLayer.sourceCropf     = mSourceCropf;

    hwLayer.displayFrame    = mDisplayFrame;

#if ANDROID_VERSION >= 430  // >= 4.3.x
    hwLayer.planeAlpha = mPlaneAlpha;
#endif

    hwLayer.acquireFenceFd = -1; // Acquire fences must only be set for Overlays and FRAMEBUFFER_TARGET
                                 // so we must wait until after Prepare to do this.
    hwLayer.releaseFenceFd = -1; // This will be returned by HWC.
}
#endif
hwc_rect_t* Hwch::Layer::AssignVisibleRegions(hwc_rect_t* visibleRegions,
                                              uint32_t& visibleRegionCount) {
  // Allocate space for visibleRegionScreen
  int numRects = mVisibleRegion.size();

  if ((visibleRegionCount + numRects) > MAX_VISIBLE_REGIONS) {
    printf("FATAL ERROR - MAX VISIBLE REGION COUNT EXCEEDED");
    exit(1);
  }

  hwc_rect_t* vr_rects = visibleRegions + visibleRegionCount;

  if (numRects == 0) {
    vr_rects->left = mDisplayFrame.left;
    vr_rects->top = mDisplayFrame.top;
    vr_rects->right = mDisplayFrame.right;
    vr_rects->bottom = mDisplayFrame.bottom;
    ++visibleRegionCount;
  } else {
    // Copy visibleRegionScreen to dc->hwLayers[i].visibleRegionScreen
    for (int r = 0; r < numRects; r++) {
      vr_rects[r].left = mVisibleRegion[r].left;
      vr_rects[r].top = mVisibleRegion[r].top;
      vr_rects[r].right = mVisibleRegion[r].right;
      vr_rects[r].bottom = mVisibleRegion[r].bottom;
    }
    visibleRegionCount += numRects;
  }
  return vr_rects;
}

buffer_handle_t Hwch::Layer::Send() {
  // The handle from the record file is now the key
  buffer_handle_t handle;
  HWCLOGV_COND(eLogHarness, "Sending layer %s @%p", mName.string(), this);

  bool patternNeedsUpdate = mPattern.get() && mPattern->FrameNeedsUpdate();
  bool buffersFilledAtLeastOnce =
      mBufs.get() && mBufs->BuffersFilledAtLeastOnce();

  if ((mBufs.get() && mPattern.get() && mBufs->NeedsUpdating()) &&
      (patternNeedsUpdate || !buffersFilledAtLeastOnce)) {
    HWCLOGD_COND(eLogHarness, "Layer %s setting updatedSinceLastFBComp",
                 mName.string());
    mPattern->SetUpdatedSinceLastFBComp();
    handle = mBufs->GetNextBuffer();

    if (HasNV12Format()) {
      mBufs->AdvanceTimestamp(mPattern->GetUpdatePeriodNs());
    }

    mBufs->WaitReleaseFence(mSystem.GetFenceTimeout(), mName);

    mPattern->Advance();

    if (!mSystem.IsFillDisabled()) {
      android::sp<android::GraphicBuffer> buf = mBufs->Get();

      if (buf.get()) {
        // Update the Render Compression resolve state in Gralloc
        UpdateRCResolve();
        mPattern->Fill(buf, Rect(0, 0, buf->getWidth(), buf->getHeight()),
                       mBufs->GetInstanceParam());
      } else {
        HWCLOGW("Layer %s current buffer is null so no fill", mName.string());
      }
    }

    HWCLOGV_COND(eLogHarness, "Layer %s filled", mName.string());
  } else if (mBufs.get()) {
    // No update, use existing buffer
    handle = mBufs->GetHandle();
    HWCLOGV_COND(eLogHarness, "Layer %s skipped", mName.string());
  } else {
    if ((mFlags & HWC_SKIP_LAYER) == 0) {
      HWCLOGW("Layer @ %p has no buffer.", this);
    }

    handle = 0;
  }

  // Setup the layer properties and the visible regions
  // AssignLayerProperties(hwLayer, handle);
  // AssignVisibleRegions(hwLayer, visibleRegions, visibleRegionCount);

  if (mPattern.get() && (mPattern->IsAllTransparent())) {
    HWCLOGV_COND(eLogHarness, "SetFutureTransparentLayer %p", handle);
    HwcTestState::getInstance()->SetFutureTransparentLayer(handle);
  }
  return handle;
}

void Hwch::Layer::UpdateRCResolve() {
#ifdef HWCVAL_ENABLE_RENDER_COMPRESSION
  // Update the resolve state in Gralloc (if this layer is marked as compressed)
  if (mCompressionType != eCompressionAuto) {
    using namespace ::intel::ufo::gralloc;

    Hwcval::buffer_details_t bufferInfo;
    GrallocClient& gc = GrallocClient::getInstance();

    if (mBufs.get()) {
      // See if this buffer has an Aux buffer
      gc.getBufferInfo(mBufs->GetHandle(), &bufferInfo);
      uint32_t size = bufferInfo.size;
      const uint32_t stride = bufferInfo.rc.aux_pitch;

      if (!((stride == 0) || (bufferInfo.rc.aux_offset >= size))) {
        // Found an Aux buffer - set the Render Compression flag in the buffer
        // details
        intel_ufo_buffer_resolve_details_t rd;
        rd.magic = sizeof(intel_ufo_buffer_resolve_details_t);
        HWCLOGV_COND(eLogRenderCompression,
                     "UpdateRCResolve: Buffer %p has aux, compression type %s",
                     mBufs->GetHandle(), CompressionTypeStr(mCompressionType));

        if (gc.getBufferResolveDetails(mBufs->GetHandle(), &rd) == 0) {
          if ((mCompressionType == eCompressionRC) ||
              (mCompressionType == eCompressionCC_RC)) {
            rd.state = INTEL_UFO_BUFFER_STATE_COMPRESSED;
          } else if (mCompressionType == eCompressionHint) {
            if (FormatSupportsRC() && !IsEncrypted()) {
              // Set the state based on the RC hint. If we deliberately want to
              // ignore
              // the hint, then use the opposite value.
              switch (rd.hint) {
                case INTEL_UFO_BUFFER_HINT_RC_FULL_RESOLVE: {
                  rd.state =
                      mSystem.IsRCHintToBeIgnored()
                          ? INTEL_UFO_BUFFER_STATE_COMPRESSED /* i.e. not
                                                                 resolved */
                          : INTEL_UFO_BUFFER_STATE_NO_CONTENT;
                  HWCLOGD_COND(eLogRenderCompression,
                               "Hint FULL RESOLVE, state -> %s",
                               AuxBufferStateStr(rd.state));
                  break;
                }
                case INTEL_UFO_BUFFER_HINT_RC_PARTIAL_RESOLVE:
                case INTEL_UFO_BUFFER_HINT_RC_DISABLE_RESOLVE: {
                  rd.state =
                      mSystem.IsRCHintToBeIgnored()
                          ? INTEL_UFO_BUFFER_STATE_NO_CONTENT /* i.e. resolved
                                                                 */
                          : INTEL_UFO_BUFFER_STATE_COMPRESSED;
                  HWCLOGD_COND(eLogRenderCompression,
                               "Hint PARTIAL/DISABLE RESOLVE, state -> %s",
                               AuxBufferStateStr(rd.state));
                  break;
                }
                default: {
                  HWCLOGW("Saw unhandled RC hint 0x%x - not changing RC state",
                          rd.hint);
                }
              };
            } else {
              HWCLOGD_COND(
                  eLogRenderCompression,
                  "Buffer %p: RC hint ignored because %s %s",
                  mBufs->GetHandle(),
                  FormatSupportsRC() ? "" : "format does not support RC",
                  IsEncrypted() ? "buffer is encrypted" : "");
              rd.state = INTEL_UFO_BUFFER_STATE_AUX_DISABLED;
            }
          }

          gc.setBufferResolveDetails(mBufs->GetHandle(), &rd);
        }
      }
    }
  }
#endif
}

void Hwch::Layer::SetAcquireFence(
    hwc2_layer_t& hwLayer, android::sp<Hwch::TimelineThread>& timelineThread,
    int mergeFence) {
#if 0
    if (mHwcAcquireDelay > 0)
    {
        // Get time at which fence will signal, in timeline units (ms at time of writing, but could be changed in HwchTimelineThread).
        uint32_t fenceSignalTime = timelineThread->GetTimelineTime() + mHwcAcquireDelay;
        int fence = sw_sync_fence_create(timelineThread->Get(), mName.string(), fenceSignalTime);
        HWCLOGD_COND(eLogTimeline, "Hwch::Layer::SetAcquireFence Created fence %d for layer %s @ delay %d, fence signal time=%d",
            fence, mName.string(), mHwcAcquireDelay, fenceSignalTime);

        if (mergeFence > 0)
        {
            hwLayer.acquireFenceFd = sync_merge("Hwch final merged fence", fence, mergeFence);
            HWCLOGD_COND(eLogTimeline, "Hwch::Layer::SetAcquireFence fence %d=%d+%d for layer %s",
                hwLayer.acquireFenceFd, fence, mergeFence, mName.string());
            CloseFence(fence);
            CloseFence(mergeFence);
        }
        else
        {
            hwLayer.acquireFenceFd = fence;
        }
     }
    else if (mergeFence > 0)
    {
        HWCLOGD_COND(eLogTimeline, "Hwch::Layer::SetAcquireFence Applied merge fence %d to layer %s", mergeFence, mName.string());
        hwLayer.acquireFenceFd = mergeFence;
    }
#endif
}

void Hwch::Layer::PostFrame(uint32_t compType, int releaseFenceFd) {
  mCurrentCompType = compType;
#if 0

    if (mBufs.get())
    {
        mBufs->PostFrame(releaseFenceFd);
    }
#endif
}

void Hwch::Layer::DoCloning(Layer** lastClonedLayer, Frame* frame) {
  if (IsForCloning()) {
    HWCLOGD_COND(eLogCloning, "Layer %s is for cloning", mName.string());
    for (uint32_t disp = 1; disp < MAX_DISPLAYS; ++disp) {
      if (mSystem.GetDisplay(disp).IsConnected()) {
        Layer* clonedLayer = mClonedLayers[disp];

        if (clonedLayer == 0) {
          // Layer not previously cloned
          HWCLOGI_COND(eLogLayerAlloc, "Layer %s new clone", mName.string());
          clonedLayer = Dup();
          mClonedLayers[disp] = clonedLayer;
          mSystem.GetDisplay(disp).CloneTransform(*this, *clonedLayer);
          if (lastClonedLayer[disp] == 0) {
            frame->AddBefore(0, *clonedLayer, disp);
          } else {
            frame->AddAfter(lastClonedLayer[disp], *clonedLayer, disp);
          }
        } else if (IsGeometryChanged()) {
          // Layer already cloned, but co-ordinates or transform have changed
          HWCLOGD_COND(eLogCloning, "Layer %s update cloning", mName.string());
          mSystem.GetDisplay(disp).CloneTransform(*this, *clonedLayer);
        } else {
          HWCLOGD_COND(eLogCloning, "Layer %s no change to cloning",
                       mName.string());
        }
        lastClonedLayer[disp] = clonedLayer;
      } else if (mClonedLayers[disp]) {
        // Layer WAS cloned, but should be no longer
        HWCLOGD_COND(eLogLayerAlloc, "Layer %s delete clone D%d",
                     mName.string(), disp);
        delete mClonedLayers[disp];
        mClonedLayers[disp] = 0;

        // The chain of events started by the deletion of the cloned layer will
        // have reset our mIsForCloning flag
        // So fix this.
        mIsForCloning = true;
      }
    }
  }
}

// Provided to make subclass code simpler
uint32_t Hwch::Layer::GetPanelWidth() {
  return System::getInstance().GetDisplay(0).GetWidth();
}

uint32_t Hwch::Layer::GetPanelHeight() {
  return System::getInstance().GetDisplay(0).GetHeight();
}

Hwch::Layer& Hwch::Layer::SetGeometryChanged(bool changed) {
  mGeometryChanged = changed;
  return *this;
}

bool Hwch::Layer::IsGeometryChanged() {
  return mGeometryChanged;
}

bool Hwch::Layer::IsForCloning() {
  return mIsForCloning;
}

Hwch::Layer& Hwch::Layer::SetForCloning(bool forCloning) {
  mIsForCloning = forCloning;
  mGeometryChanged = true;
  return *this;
}

Hwch::Layer& Hwch::Layer::SetFrame(Frame* frame) {
  mFrame = frame;
  return *this;
}

Hwch::Frame* Hwch::Layer::GetFrame() {
  return mFrame;
}

void Hwch::Layer::RemoveClones() {
  for (uint32_t disp = 0; disp < MAX_DISPLAYS; ++disp) {
    if (mClonedLayers[disp]) {
      delete mClonedLayers[disp];
      mClonedLayers[disp] = 0;
    }
  }

  // Force update of cloning
  mGeometryChanged = true;
}

Hwch::Layer* Hwch::Layer::RemoveClone(Hwch::Layer* cloneToRemove) {
  Hwch::Layer* clone = 0;
  for (uint32_t disp = 0; disp < MAX_DISPLAYS; ++disp) {
    if (mClonedLayers[disp]) {
      if (cloneToRemove == mClonedLayers[disp]) {
        clone = mClonedLayers[disp];
        mClonedLayers[disp] = 0;
      }
    }
  }

  // Force update of cloning
  mGeometryChanged = true;
  return clone;
}

void Hwch::Layer::SendToFront() {
  // Delete any clones so they have to be recreated
  if (mFrame) {
    RemoveClones();
    Frame* frame = mFrame;
    frame->Remove(*this);
    frame->Add(*this);
  }
}

void Hwch::Layer::SendToBack() {
  if (mFrame) {
    Hwch::Frame* frame = mFrame;
    RemoveClones();

    uint32_t ix;
    uint32_t disp;
    if (frame->FindLayer(*this, ix, disp)) {
      if (ix != 0) {
        frame->RemoveLayerAt(ix, disp);
        frame->InsertLayerAt(*this, 0, disp);
      }
    }
  }
}

void Hwch::Layer::SendForward() {
  if (mFrame) {
    Hwch::Frame* frame = mFrame;
    RemoveClones();
    uint32_t ix;
    uint32_t disp;
    if (frame->FindLayer(*this, ix, disp)) {
      if (ix < frame->NumLayers(disp) - 1) {
        frame->RemoveLayerAt(ix, disp);
        frame->InsertLayerAt(*this, ix + 1, disp);
      }
    }
  }
}

void Hwch::Layer::SendBackward() {
  if (mFrame) {
    Hwch::Frame* frame = mFrame;
    RemoveClones();
    uint32_t ix;
    uint32_t disp;
    if (frame->FindLayer(*this, ix, disp)) {
      if (ix > 0) {
        frame->RemoveLayerAt(ix, disp);
        frame->InsertLayerAt(*this, ix - 1, disp);
      }
    }
  }
}

void Hwch::Layer::CalculateDisplayFrame(Display& display) {
  if ((mLogicalDisplayFrame.right.mType == eCoordUnassigned) ||
      (mLogicalDisplayFrame.bottom.mType == eCoordUnassigned)) {
    LogDisplayRect ldf = mLogicalDisplayFrame;

    if (ldf.right.mType == eCoordUnassigned) {
      ldf.right = ldf.left;
      ldf.right.mValue += (mSourceCropf.right - mSourceCropf.left);
    }

    if (ldf.bottom.mType == eCoordUnassigned) {
      ldf.bottom = mLogicalDisplayFrame.top;
      ldf.bottom.mValue += (mSourceCropf.bottom - mSourceCropf.top);
    }

    if (mIgnoreScreenRotation) {
      display.CopyRect(mFormat, ldf, mDisplayFrame);
    } else {
      display.ConvertRect(mFormat, ldf, mDisplayFrame);
    }
  } else {
    if (mIgnoreScreenRotation) {
      display.CopyRect(mFormat, mLogicalDisplayFrame, mDisplayFrame);
    } else {
      display.ConvertRect(mFormat, mLogicalDisplayFrame, mDisplayFrame);
    }
  }

  if (mOldDisplayFrame.left != mDisplayFrame.left ||
      mOldDisplayFrame.top != mDisplayFrame.top ||
      mOldDisplayFrame.right != mDisplayFrame.right ||
      mOldDisplayFrame.bottom != mDisplayFrame.bottom) {
    SetUpdated();
  }
  mOldDisplayFrame = mDisplayFrame;
}

void Hwch::Layer::CalculateSourceCrop(Display& display) {
  uint32_t width;
  uint32_t height;

  if (!mIgnoreScreenRotation) {
    width = mWidth.Phys(display.GetLogicalWidth());
    height = mHeight.Phys(display.GetLogicalHeight());

    if (mLogicalCropf.right.mType == eCoordUnassigned) {
      mSourceCropf.left = 0;
      mSourceCropf.top = 0;
      mSourceCropf.right = width;
      mSourceCropf.bottom = height;
    } else {
      mSourceCropf.left = mLogicalCropf.left.Phys(display.GetLogicalWidth());
      mSourceCropf.top = mLogicalCropf.top.Phys(display.GetLogicalHeight());
      mSourceCropf.right = mLogicalCropf.right.Phys(display.GetLogicalWidth());
      mSourceCropf.bottom =
          mLogicalCropf.bottom.Phys(display.GetLogicalHeight());
    }
  } else {
    // Ignoring screen rotation
    width = mWidth.Phys(display.GetWidth());
    height = mHeight.Phys(display.GetHeight());

    if (mLogicalCropf.right.mType == eCoordUnassigned) {
      mSourceCropf.left = 0;
      mSourceCropf.top = 0;
      mSourceCropf.right = width;
      mSourceCropf.bottom = height;
    } else {
      mSourceCropf.left = mLogicalCropf.left.Phys(display.GetWidth());
      mSourceCropf.top = mLogicalCropf.top.Phys(display.GetHeight());
      mSourceCropf.right = mLogicalCropf.right.Phys(display.GetWidth());
      mSourceCropf.bottom = mLogicalCropf.bottom.Phys(display.GetHeight());
    }
  }

  // Enforce crop alignment / min / max rules
  uint32_t bw = INT_MAX;
  uint32_t bh = INT_MAX;

  if (mBufs.get()) {
    bw = mBufs->GetWidth();
    bh = mBufs->GetHeight();
  }

  float w = mSourceCropf.right - mSourceCropf.left;
  float h = mSourceCropf.bottom - mSourceCropf.top;

  mSystem.GetBufferFormatConfigManager().AdjustCrop(
      mFormat, bw, bh, mSourceCropf.left, mSourceCropf.top, w, h);

  mSourceCropf.right = mSourceCropf.left + w;
  mSourceCropf.bottom = mSourceCropf.top + h;

  // Detect changes
  if (mOldSourceCropf.left != mSourceCropf.left ||
      mOldSourceCropf.top != mSourceCropf.top ||
      mOldSourceCropf.right != mSourceCropf.right ||
      mOldSourceCropf.bottom != mSourceCropf.bottom) {
    SetUpdated();
  }
  mOldSourceCropf = mSourceCropf;
}

void Hwch::Layer::CalculateRects(Display& display) {
  mPhysicalTransform = display.RotateTransform(mLogicalTransform);

  CalculateSourceCrop(display);
  CalculateDisplayFrame(display);

  HWCLOGI_COND(eLogHarness, "CalculateRects(%s): LogCrop %s Crop %s",
               mName.string(), mLogicalCropf.Str("%f"), mSourceCropf.Str());
  HWCLOGI_COND(eLogHarness, "CalculateRects(%s): LogDF   %s DF   %s",
               mName.string(), mLogicalDisplayFrame.Str("%d"),
               mDisplayFrame.Str());

  // Create buffers if new, or size has changed (old buffers will be
  // dereferenced)
  uint32_t width = mWidth.Phys(display.GetLogicalWidth());
  uint32_t height = mHeight.Phys(display.GetLogicalHeight());

  if (mNeedBuffer && ((mBufs.get() == 0) || (mBufs->GetWidth() != width) ||
                      (mBufs->GetHeight() != height))) {
    // Overriding option values in this way does not work a way of creating
    // X-tiled buffers when we want them
    // What we really need is for the y-tiling to be turned off at the moment
    // when HWC calls drmModeAddFb for
    // the buffer. This will be some time later and is not under our control.
    //
    // The effect of this code when we were using it was therefore to create
    // some X-tiled buffers at random
    // depending on what HWC threads happen to be doing when we turn the fbytile
    // option off. This can cause
    // assertions in HWC because for example you can end up with some members of
    // a buffer set X tiled while
    // others are Y tiled. The X tiled ones don't support RC and therefore a
    // "geometry change required" assertion
    // results when we rotate the buffers.
    //// Can't set the option value before HWC has created it
    // mSystem.OverrideTile(mTile);
    uint32_t usage = mUsage;
    if (mTile == eLinear) {
      usage |= GRALLOC_USAGE_SW_WRITE_MASK;
    }

    mBufs = new BufferSet(width, height, mFormat, mNumBuffers, usage);
    // mSystem.ResetTile();
    if (mBufs->GetHandle() == 0) {
      HWCERROR(eCheckTestBufferAlloc, "Failed to create buffers for layer %s",
               mName.string());
    }

#ifdef HWCVAL_BUILD_PAVP
    if (mEncrypted != eNotEncrypted) {
      uint32_t sessionId = Hwch::System::getInstance().GetPavpSessionId();
      if (mEncrypted & eInvalidSessionId) {
        ++sessionId;
      }

      uint32_t instance = Hwch::System::getInstance().GetPavpInstance();
      if (mEncrypted & eInvalidInstanceId) {
        // Instance must be in the valid range for this test to work
        instance = (instance + 8) % 15;
      }

      mBufs->SetProtectionState(true, sessionId, instance);
    }
#endif
  }

  if (!((mSourceCropf.left >= 0) && (mSourceCropf.top >= 0) &&
        (mSourceCropf.left < mSourceCropf.right) &&
        (mSourceCropf.top < mSourceCropf.bottom) &&
        (mSourceCropf.right <= width) && (mSourceCropf.bottom <= height)) &&
      ((mFlags & HWC_SKIP_LAYER) ==
       0))  // Don't check crop for SKIP layers, these are (0,0,0,0).
  {
    ALOGE("Layer %s: Bad crop %s", GetName(), mSourceCropf.Str());
    ALOG_ASSERT(mSourceCropf.left >= 0);
    ALOG_ASSERT(mSourceCropf.top >= 0);
    ALOG_ASSERT(mSourceCropf.left < mSourceCropf.right);
    ALOG_ASSERT(mSourceCropf.top < mSourceCropf.bottom);
    ALOG_ASSERT(mSourceCropf.right <= width);
    ALOG_ASSERT(mSourceCropf.bottom <= height);
  }
}

// For FramebufferTarget
// Fill buffer excluding a specified rectangle
// Used to fill black around the bottom layer as this is likely to be most or
// all of the screen.

void Hwch::Layer::FillExcluding(const hwc_rect_t& rect,
                                const hwc_rect_t& exclRect) {
  uint32_t bufferParam = HWCH_BUFFERPARAM_UNDEFINED;
  if (exclRect.top > rect.bottom) {
    mPattern->Fill(mBufs->Get(), rect, bufferParam);
  } else {
    uint32_t t = rect.top;
    uint32_t b = rect.bottom;

    if (exclRect.top > rect.top) {
      t = exclRect.top;
      mPattern->Fill(mBufs->Get(),
                     Rect(rect.left, rect.top, rect.right, exclRect.top),
                     bufferParam);
    }

    if (exclRect.bottom < rect.bottom) {
      b = exclRect.bottom;
      mPattern->Fill(mBufs->Get(),
                     Rect(rect.left, exclRect.bottom, rect.right, rect.bottom),
                     bufferParam);
    }

    if (exclRect.left > rect.left) {
      mPattern->Fill(mBufs->Get(), Rect(rect.left, t, exclRect.left, b),
                     bufferParam);
    }

    if (exclRect.right < rect.right) {
      mPattern->Fill(mBufs->Get(), Rect(exclRect.right, t, rect.right, b),
                     bufferParam);
    }
  }
}

const char* Hwch::Layer::GetName() {
  return mName.string();
}

float Hwch::Layer::GetBytesPerPixel() {
  // set pixel format
  switch (mFormat) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
    case HAL_PIXEL_FORMAT_RGBX_8888:
    case HAL_PIXEL_FORMAT_BGRA_8888:
      return 4;

    case HAL_PIXEL_FORMAT_RGB_888:
      return 3;

    case HAL_PIXEL_FORMAT_RGB_565:
      return 2;

    case HAL_PIXEL_FORMAT_YV12:
    case HAL_PIXEL_FORMAT_NV12_X_TILED_INTEL:
    case HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL:
    case HAL_PIXEL_FORMAT_NV12_LINEAR_INTEL:
    case HAL_PIXEL_FORMAT_NV12_LINEAR_PACKED_INTEL:
    case HAL_PIXEL_FORMAT_NV12_LINEAR_CAMERA_INTEL:
    case HAL_PIXEL_FORMAT_YUV420PackedSemiPlanar_Tiled_INTEL:
      // N.B. NV12 is a complicated format with a total memory usage of 1.5
      // bytes per pixel.
      return 1.5;

    case HAL_PIXEL_FORMAT_YCbCr_422_I:
      return 2;

    default:
      printf("UNSUPPORTED PIXEL FORMAT %d\n", mFormat);
      ALOG_ASSERT(0);
  }
}

uint32_t Hwch::Layer::GetMemoryUsage() {
  ALOG_ASSERT(mWidth.mType == eCoordAbsolute);
  ALOG_ASSERT(mHeight.mType == eCoordAbsolute);
  return (GetBytesPerPixel() * mNumBuffers) * (mWidth.mValue * mHeight.mValue);
}

void Hwch::Layer::SetEncrypted(uint32_t encrypted) {
  mEncrypted = encrypted;
}

bool Hwch::Layer::IsEncrypted() {
  return (mEncrypted != eNotEncrypted);
}

// Checks if a layer is fullscreen or not
bool Hwch::Layer::IsFullScreenRotated(Hwch::Display& display) {
  int32_t width = display.GetWidth();
  int32_t height = display.GetHeight();

  return (((mDisplayFrame.top == 0) && (mDisplayFrame.bottom == height)) ||
          ((mDisplayFrame.left == 0) && (mDisplayFrame.right == width)));
}

bool Hwch::Layer::HasNV12Format() {
  return ((mFormat == HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL) ||
          (mFormat == HAL_PIXEL_FORMAT_NV12_X_TILED_INTEL) ||
          (mFormat == HAL_PIXEL_FORMAT_NV12_LINEAR_INTEL) ||
          (mFormat == HAL_PIXEL_FORMAT_NV12_LINEAR_PACKED_INTEL));
}

const char* Hwch::Layer::CompressionTypeStr(Hwch::Layer::CompressionType ct) {
  switch (ct) {
    case eCompressionAuto:
      return "AUTO";
    case eCompressionRC:
      return "RC";
    case eCompressionCC_RC:
      return "CC_RC";
    case eCompressionHint:
      return "Follow HINT";
    default:
      return "INVALID";
  }
}

const char* Hwch::Layer::AuxBufferStateStr(int state) {
#ifdef HWCVAL_ENABLE_RENDER_COMPRESSION
  switch (state) {
    case INTEL_UFO_BUFFER_STATE_AUX_DISABLED:
      return "DISABLED";
    case INTEL_UFO_BUFFER_STATE_COMPRESSED:
      return "COMPRESSED";
    default:
      return "INVALID";
  }
#else
  HWCVAL_UNUSED(state);
  return "RC not supported";
#endif
}
