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

#include "graphics.h"

#include "DrmShimBuffer.h"
#include "BufferObject.h"
#include <cutils/log.h>
#include "HwcTestState.h"
#include "DrmShimPlane.h"
#include "DrmShimChecks.h"
#include "HwcTestUtil.h"
#include "HwcTestDebug.h"

#include "drm_fourcc.h"
#include "SSIMUtils.h"
#include "cros_gralloc/cros_gralloc_handle.h"
#include <math.h>
#include <utils/Atomic.h>

using namespace Hwcval;

uint32_t DrmShimBuffer::mCount = 0;
uint32_t DrmShimBuffer::mCompMismatchCount = 0;
static int sNumBufCopies = 0;
GrallocInterface gralloc_interface;

static void InitBufferInfo(Hwcval::buffer_details_t* details) {
  // Default all buffer info state
  details->width = 0;
  details->height = 0;
  details->format = 0;
  details->usage = 0;

#if INTEL_UFO_GRALLOC_HAVE_PRIME
  details->prime = 0;
#endif

  details->fb = 0;
  details->fb_format = 0;
  details->pitch = 0;
  details->size = 0;
  details->allocWidth = 0;
  details->allocHeight = 0;
  details->allocOffsetX = 0;
  details->allocOffsetY = 0;

#if INTEL_UFO_GRALLOC_HAVE_BUFFER_DETAILS_1
  details->rc.aux_pitch = 0;
  details->rc.aux_offset = 0;
#endif
}

GrallocInterface::GrallocInterface() {
  hw_device_t* device;
  int ret = -1;
  ret =
      hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t**)&gralloc);
  if (ret) {
    ALOGE("Failed to get gralloc module");
    return;
  }

  gralloc_version = gralloc->module_api_version;
  ALOGE("gralloc version in use: %d", gralloc_version);
#ifdef HWCVAL_ENABLE_GRALLOC1
  if (gralloc_version == HARDWARE_MODULE_API_VERSION(1, 0)) {
    ret = gralloc->methods->open(gralloc, GRALLOC_HARDWARE_MODULE_ID, &device);
    if (ret) {
      ALOGE("Failed to open hw_device device");
      return;
    } else {
      gralloc1_dvc = (gralloc1_device_t*)device;

      pfn_getFormat = (GRALLOC1_PFN_GET_FORMAT)gralloc1_dvc->getFunction(
          gralloc1_dvc, GRALLOC1_FUNCTION_GET_FORMAT);

      pfn_getDimensions =
          (GRALLOC1_PFN_GET_DIMENSIONS)gralloc1_dvc->getFunction(
              gralloc1_dvc, GRALLOC1_FUNCTION_GET_DIMENSIONS);

      pfn_getStride =
          (GRALLOC1_PFN_GET_STRIDE)gralloc1_dvc->getFunction(
              gralloc1_dvc, GRALLOC1_FUNCTION_GET_STRIDE);
    }
  }
#endif
}

int hwc_buffer_details::getBufferHandles(buffer_handle_t handle, uint32_t *handles) {
  ALOGE("handle BufferInfo %llu", handle);
  int ret = -1;
  if (gralloc_interface.gralloc_version == HARDWARE_MODULE_API_VERSION(1, 0)) {
   struct cros_gralloc_handle *hnd = (struct cros_gralloc_handle *)handle;
   for (size_t plane = 0; plane < DRV_MAX_PLANES; plane++)
              handles[plane] = hnd->fds[plane];
  }
  return 0;
}

int hwc_buffer_details::getBufferInfo(buffer_handle_t handle) {
  ALOGE("handle BufferInfo %llu", handle);
  int ret = -1;
#ifdef HWCVAL_ENABLE_GRALLOC1
  if (gralloc_interface.gralloc_version == HARDWARE_MODULE_API_VERSION(1, 0)) {
    if (!gralloc_interface.pfn_getDimensions) {
      ALOGE("Gralloc does not support getDimension");
      return -1;
    }
    ret = gralloc_interface.pfn_getDimensions(gralloc_interface.gralloc1_dvc,
                                              handle, &width, &height);
    if (ret) {
      ALOGE("gralloc->getDimension failed: %d", ret);
      return -1;
    }

    if (!gralloc_interface.pfn_getStride) {
      ALOGE("Gralloc does not support getStride");
      return -1;
    }
    ret = gralloc_interface.pfn_getStride(gralloc_interface.gralloc1_dvc,
                                              handle, &pitch);
    if (ret) {
      ALOGE("gralloc->getiStride failed: %d", ret);
      return -1;
    }

    if (!gralloc_interface.pfn_getFormat) {
      ALOGE("Gralloc does not support getFormat");
      return -1;
    }
    ret = gralloc_interface.pfn_getFormat(gralloc_interface.gralloc1_dvc,
                                          handle, &format);
    if (ret) {
      ALOGE("gralloc->getFormat failed: %d", ret);
      return -1;
    }
  } else {
#endif
    gralloc_module_t* gralloc0;
    gralloc0 = reinterpret_cast<gralloc_module_t*>(gralloc_interface.gralloc);

    if (!gralloc0->perform) {
      ALOGE("gralloc->perform not supported");
      return -1;
    }
    ret = gralloc0->perform(gralloc0, GRALLOC_DRM_GET_FORMAT, handle, &format);
    if (ret) {
      ALOGE("gralloc->perform failed with error: %d", ret);
      return -1;
    }
    ret = gralloc0->perform(gralloc0, GRALLOC_DRM_GET_DIMENSIONS, handle,
                            &width, &height);
    if (ret) {
      ALOGE("gralloc->perform failed with error: %d", ret);
      return -1;
    }
#ifdef HWCVAL_ENABLE_GRALLOC1
  }
#endif
  return 0;
}

int DrmShimBuffer::GetBufferInfo(buffer_handle_t handle,
                                 Hwcval::buffer_details_t* details) {
  InitBufferInfo(details);
  return details->getBufferInfo(handle);
}

// Usual constructor, when we recognise a new buffer passed into OnSet
DrmShimBuffer::DrmShimBuffer(buffer_handle_t handle,
                             Hwcval::BufferSourceType bufferSource)
    : mHandle(handle),
      mDsId(0),
      mAcquireFenceFd(-1),
      mNew(true),
      mUsed(false),
      mBufferSource(bufferSource),
      mBlanking(false),
      mBlack(false),
      mFbtDisplay(-1),
      mReallyProtected(false),
      mTransparentFromHarness(false),
      mBufferIx(0),
      mToBeCompared(0),
      mShouldBeProtected(eProtDontCare),
      mAppearanceCount(0),
      mBufferContent(Hwcval::BufferContentType::ContentNotTested) {
  InitBufferInfo(&mDetails);
  memset(&mMediaDetails, 0, sizeof(hwc_buffer_media_details_t));
#ifdef HWCVAL_ENABLE_RENDER_COMPRESSION
  memset(&mResolveDetails, 0, sizeof(hwc_buffer_resolve_details_t));
#endif
  for (uint32_t i = 0; i < HWCVAL_MAX_CRTCS; ++i) {
    mLastHwcFrame[i] = 0xfffffffe;
    mLastOnSetFrame[i] = 0;
  }

  ++mCount;
  HWCLOGD_COND(eLogBuffer, "DrmShimBuffer::DrmShimBuffer Created buf@%p", this);
}

DrmShimBuffer::~DrmShimBuffer() {
  if (mAcquireFenceFd > 0) {
    CloseFence(mAcquireFenceFd);
  }

  FreeBufCopies();

  --mCount;
  HWCLOGD_COND(eLogBuffer, "DrmShimBuffer::~DrmShimBuffer Deleted buf@%p",
               this);
}

void DrmShimBuffer::FreeBufCopies() {
  if (mBufCpy.get()) {
    --sNumBufCopies;
  }

  mBufCpy = 0;
  mRef = 0;
}

buffer_handle_t DrmShimBuffer::GetHandle() const {
  return mHandle;
}

// To be used with care - i.e. only when not already in the handle index.
DrmShimBuffer* DrmShimBuffer::SetHandle(buffer_handle_t handle) {
  mHandle = handle;
  UpdateMediaDetails();
  UpdateResolveDetails();

  return this;
}

bool DrmShimBuffer::IsOpen() {
  return (GetOpenCount() != 0);
}

uint32_t DrmShimBuffer::GetOpenCount() {
  return mBos.size();
}

DrmShimBuffer* DrmShimBuffer::AddBo(android::sp<HwcTestBufferObject> bo) {
  mBos.add(bo);
  return this;
}

DrmShimBuffer* DrmShimBuffer::RemoveBo(android::sp<HwcTestBufferObject> bo) {
  char strbuf[HWCVAL_DEFAULT_STRLEN];
  char strbuf2[HWCVAL_DEFAULT_STRLEN];

  ssize_t ix = mBos.indexOf(bo);

  if (ix >= 0) {
    mBos.removeAt(ix);
  } else {
    HWCLOGI_COND(eLogBuffer, "DrmShimBuffer::RemoveBo %s not found in %s",
                 bo->IdStr(strbuf), IdStr(strbuf2));
  }

  return this;
}

DrmShimBuffer* DrmShimBuffer::RemoveBo(int fd, uint32_t boHandle) {
  for (uint32_t i = 0; i < mBos.size(); ++i) {
    android::sp<HwcTestBufferObject> bo = mBos.itemAt(i);

    if ((bo->mFd == fd) && (bo->mBoHandle == boHandle)) {
      mBos.removeAt(i);
      break;
    }
  }

  return this;
}

HwcTestBufferObjectVector& DrmShimBuffer::GetBos() {
  return mBos;
}

DrmShimBuffer* DrmShimBuffer::SetNew(bool isNew) {
  mNew = isNew;
  return this;
}

bool DrmShimBuffer::IsNew() {
  return mNew;
}

DrmShimBuffer* DrmShimBuffer::SetUsed(bool used) {
  mUsed = used;
  return this;
}

bool DrmShimBuffer::IsUsed() {
  return mUsed;
}

DrmShimBuffer* DrmShimBuffer::SetCompositionTarget(
    Hwcval::BufferSourceType bufferSource) {
  mBufferSource = bufferSource;
  return this;
}

Hwcval::BufferSourceType DrmShimBuffer::GetSource() {
  return mBufferSource;
}

bool DrmShimBuffer::IsCompositionTarget() {
  return ((mBufferSource != Hwcval::BufferSourceType::Input) &&
          (mBufferSource != Hwcval::BufferSourceType::Hwc));
}

DrmShimBuffer* DrmShimBuffer::SetBlanking(bool blanking) {
  mBlanking = blanking;
  return this;
}

bool DrmShimBuffer::IsBlanking() {
  return mBlanking;
}

DrmShimBuffer* DrmShimBuffer::SetBlack(bool black) {
  mBlack = black;
  return this;
}

bool DrmShimBuffer::IsBlack() {
  return mBlack;
}

DrmShimBuffer* DrmShimBuffer::SetFbtDisplay(uint32_t displayIx) {
  mFbtDisplay = (int32_t)displayIx;
  return this;
}

bool DrmShimBuffer::IsFbt() {
  return (mFbtDisplay >= 0);
}

uint32_t DrmShimBuffer::GetFbtDisplay() {
  return (uint32_t)mFbtDisplay;
}

bool DrmShimBuffer::IsFbtDisplay0() {
  return (mFbtDisplay == 0);
}

uint32_t DrmShimBuffer::NumFbIds() const {
  return mFbIds.size();
}

DrmShimBuffer* DrmShimBuffer::SetDsId(int64_t dsId) {
  mDsId = dsId;
  return this;
}

int64_t DrmShimBuffer::GetDsId() {
  return mDsId;
}

DrmShimBuffer* DrmShimBuffer::SetDetails(
    const Hwcval::buffer_details_t& details) {
  mDetails = details;
  UpdateMediaDetails();
  UpdateResolveDetails();

  // Normally buffer protection state is unknown. This will be ovewritten if
  // this is an iVP target.
  mShouldBeProtected = eProtDontCare;
  return this;
}

const Hwcval::buffer_details_t& DrmShimBuffer::GetDetails() const {
  return mDetails;
}

// The global buffer id
// On older builds, this will be "name" - actually a number and a system-global
// ID
// On newer builds, this will be "prime" - a FD, and hence a process-global ID.
DrmShimBuffer* DrmShimBuffer::SetGlobalId(int id) {
  mDetails.NAME_OR_PRIME = id;
  return this;
}

int DrmShimBuffer::GetGlobalId() const {
  return mDetails.NAME_OR_PRIME;
}

DrmShimBuffer* DrmShimBuffer::UpdateMediaDetails() {
  // Media Details is only used on hwc-next
  char strbuf[HWCVAL_DEFAULT_STRLEN];

  if (mHandle != 0) {
    mMediaDetails.magic = sizeof(hwc_buffer_media_details_t);
    if (1 /*GetGralloc().queryMediaDetails(mHandle, &mMediaDetails) != 0*/) {
      HWCLOGW("DrmShimBuffer::UpdateMediaDetails: queryMediaDetails Failed %s",
              IdStr(strbuf));
      memset(&mMediaDetails, 0, sizeof(hwc_buffer_media_details_t));
    } 
  } else {
    memset(&mMediaDetails, 0, sizeof(hwc_buffer_media_details_t));
  }

  return this;
}

DrmShimBuffer* DrmShimBuffer::UpdateResolveDetails() {
#ifdef HWCVAL_ENABLE_RENDER_COMPRESSION
  char strbuf[HWCVAL_DEFAULT_STRLEN];

  if ((mHandle != 0) && (mDetails.rc.aux_pitch != 0) &&
      (mDetails.rc.aux_offset < mDetails.size)) {
    mResolveDetails.magic = sizeof(hwc_buffer_resolve_details_t);
    if (GetGralloc().getBufferResolveDetails(mHandle, &mResolveDetails) != 0) {
      HWCLOGW(
          "DrmShimBuffer::UpdateResolveDetails: getBufferResolveDetails failed "
          "%s",
          IdStr(strbuf));
      memset(&mResolveDetails, 0, sizeof(hwc_buffer_resolve_details_t));
    } else {
      HWCLOGV_COND(
          eLogRenderCompression,
          "DrmShimBuffer::UpdateResolveDetails getBufferResolveDetails handle "
          "%p state %s hint %s",
          mHandle,
          mResolveDetails.state == INTEL_UFO_BUFFER_STATE_AUX_DISABLED
              ? "AUX_DISABLED"
              : mResolveDetails.state == INTEL_UFO_BUFFER_STATE_NO_CONTENT
                    ? "NO_CONTENT"
                    : mResolveDetails.state == INTEL_UFO_BUFFER_STATE_COMPRESSED
                          ? "STATE_COMPRESSED"
                          : "UNKNOWN",

          mResolveDetails.hint == INTEL_UFO_BUFFER_HINT_RC_UNDEFINED
              ? "NONE"
              : mResolveDetails.hint == INTEL_UFO_BUFFER_HINT_RC_FULL_RESOLVE
                    ? "FULL RESOLVE"
                    : mResolveDetails.hint ==
                              INTEL_UFO_BUFFER_HINT_RC_PARTIAL_RESOLVE
                          ? "PARTIAL (NO CC) RESOLVE"
                          : mResolveDetails.hint ==
                                    INTEL_UFO_BUFFER_HINT_RC_DISABLE_RESOLVE
                                ? "NO RESOLVE (RC AND CC)"
                                : mResolveDetails.hint ==
                                          INTEL_UFO_BUFFER_HINT_MMC_COMPRESSED
                                      ? "MMC COMPRESSED"
                                      : "UNKNOWN");
    }
  } else {
    memset(&mResolveDetails, 0, sizeof(hwc_buffer_resolve_details_t));
  }
#endif

  return this;
}

bool DrmShimBuffer::IsRenderCompressed() {
#ifdef HWCVAL_ENABLE_RENDER_COMPRESSION
  return mResolveDetails.state == INTEL_UFO_BUFFER_STATE_COMPRESSED;
#else
  return false;
#endif
}

bool DrmShimBuffer::IsRenderCompressibleFormat() {
#ifdef HWCVAL_ENABLE_RENDER_COMPRESSION
  uint32_t format = GetFormat();
  return ((format == HAL_PIXEL_FORMAT_RGBA_8888) ||
          (format == HAL_PIXEL_FORMAT_BGRA_8888) ||
          (format == HAL_PIXEL_FORMAT_RGBX_8888));
#else
  return false;
#endif
}

const hwc_buffer_media_details_t& DrmShimBuffer::GetMediaDetails() const {
  return mMediaDetails;
}

#ifdef HWCVAL_ENABLE_RENDER_COMPRESSION
const hwc_buffer_resolve_details_t& DrmShimBuffer::GetResolveDetails() const {
  return mResolveDetails;
}
#endif

uint32_t DrmShimBuffer::GetWidth() {
  return mDetails.width;
}

uint32_t DrmShimBuffer::GetHeight() {
  return mDetails.height;
}

uint32_t DrmShimBuffer::GetAllocWidth() {
  return mDetails.allocWidth;
}

uint32_t DrmShimBuffer::GetAllocHeight() {
  return mDetails.allocHeight;
}

uint32_t DrmShimBuffer::GetUsage() {
  return mDetails.usage;
}

uint32_t DrmShimBuffer::GetFormat() const {
  return mDetails.format;
}

uint32_t DrmShimBuffer::GetDrmFormat() {
#ifdef HWCVAL_FB_BUFFERINFO_FORMAT
  return mDetails.fb_format;
#else
  return mDetails.drmformat;
#endif
}

// Determines whether this buffer is a video format
bool DrmShimBuffer::IsVideoFormat(uint32_t format) {
  return ((format == HAL_PIXEL_FORMAT_YV12) ||
          (format == HAL_PIXEL_FORMAT_NV12_X_TILED_INTEL) ||
          (format == HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL) ||
          (format == HAL_PIXEL_FORMAT_NV12_LINEAR_INTEL) ||
          (format == HAL_PIXEL_FORMAT_NV12_LINEAR_PACKED_INTEL) ||
          (format == HAL_PIXEL_FORMAT_NV12_LINEAR_CAMERA_INTEL) ||
          (format == HAL_PIXEL_FORMAT_YCbCr_422_I) ||
          (format == HAL_PIXEL_FORMAT_YUV420PackedSemiPlanar_INTEL) ||
          (format == HAL_PIXEL_FORMAT_YUV420PackedSemiPlanar_Tiled_INTEL));
}

bool DrmShimBuffer::IsVideoFormat() {
  return IsVideoFormat(mDetails.format);
}

// Determines whether this buffer is a video format
bool DrmShimBuffer::IsNV12Format(uint32_t format) {
  return ((format == HAL_PIXEL_FORMAT_NV12_X_TILED_INTEL) ||
          (format == HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL) ||
          (format == HAL_PIXEL_FORMAT_NV12_LINEAR_INTEL) ||
          (format == HAL_PIXEL_FORMAT_NV12_LINEAR_PACKED_INTEL) ||
          (format == HAL_PIXEL_FORMAT_NV12_LINEAR_CAMERA_INTEL));
}

// Determines whether this buffer is a video format
bool DrmShimBuffer::IsNV12Format() {
  return IsNV12Format(mDetails.format);
}

bool DrmShimBuffer::UpdateDetails() {
  Hwcval::buffer_details_t bi;
  char strbuf[HWCVAL_DEFAULT_STRLEN];

  if (mHandle) {
    if (GetBufferInfo(mHandle, &bi)) {
      HWCLOGW(
          "DrmShimBuffer::UpdateDetails can't update info for buf@%p handle %p",
          this, mHandle);

      // This probably means that the handle is no longer valid.
      return false;
    }

    if ((bi.allocWidth != mDetails.allocWidth) ||
        (bi.allocHeight != mDetails.allocHeight)) {
      HWCLOGD("DrmShimBuffer::UpdateDetails %s alloc %dx%d updated to %dx%d",
              IdStr(strbuf), mDetails.allocWidth, mDetails.allocHeight,
              bi.allocWidth, bi.allocHeight);
    }

    if ((bi.width != mDetails.width) || (bi.height != mDetails.height)) {
      HWCLOGD("DrmShimBuffer::UpdateDetails %s size %dx%d updated to %dx%d",
              IdStr(strbuf), mDetails.width, mDetails.height, bi.width,
              bi.height);
    }

    mDetails = bi;
  }

  return true;
}

// Return the current name/PRIME for this buffer
// so the caller can tell if it has changed since we cached it.
int DrmShimBuffer::GetCurrentGlobalId() {
  Hwcval::buffer_details_t bi;

  if (mHandle) {
    if (GetBufferInfo(mHandle, &bi)) {
      HWCLOGW("DrmShimBuffer::UpdateDetails can't update " BUFIDSTR
              " for buf@%p handle %p",
              this, mHandle);

      // This probably means that the handle is no longer valid.
      return 0;
    }

    return bi.NAME_OR_PRIME;
  }

  return 0;
}

DrmShimBuffer::FbIdData* DrmShimBuffer::GetFbIdData(uint32_t fbId) {
  ssize_t ix = mFbIds.indexOfKey(fbId);
  return (ix >= 0) ? &mFbIds.editValueAt(ix) : nullptr;
}

uint32_t DrmShimBuffer::GetPixelFormat(uint32_t fbId) {
  ssize_t ix = mFbIds.indexOfKey(fbId);

  if (ix >= 0) {
    const FbIdData& fbIdData = mFbIds.valueAt(ix);
    return fbIdData.pixelFormat;
  } else {
    return 0;
  }
}

DrmShimBuffer::FbIdVector& DrmShimBuffer::GetFbIds() {
  return mFbIds;
}

void DrmShimBuffer::AddCombinedFrom(DrmShimTransform& child) {
  mCombinedFrom.add(child);
}

DrmShimTransform* DrmShimBuffer::FirstCombinedFrom() {
  if (mCombinedFrom.size() > 0) {
    mBufferIx = 0;
    return &(mCombinedFrom.editItemAt(0));
  }
  return (DrmShimTransform*)0;
}

DrmShimTransform* DrmShimBuffer::NextCombinedFrom() {
  ++mBufferIx;
  if (mBufferIx < mCombinedFrom.size()) {
    return &(mCombinedFrom.editItemAt(mBufferIx));
  }
  return (DrmShimTransform*)0;
}

void DrmShimBuffer::RemoveCurrentCombinedFrom() {
  if (mBufferIx < mCombinedFrom.size()) {
    mCombinedFrom.removeAt(mBufferIx--);
  }
}

// Is buf one of the buffers that this one was composed from?
bool DrmShimBuffer::IsCombinedFrom(android::sp<DrmShimBuffer> buf) {
  if (buf.get() == this) {
    return true;
  } else {
    for (uint32_t i = 0; i < mCombinedFrom.size(); ++i) {
      DrmShimTransform& transform = mCombinedFrom.editItemAt(i);
      if (transform.GetBuf()->IsCombinedFrom(buf)) {
        return true;
      }
    }

    return false;
  }
}

// Recursively expand a transform using the "combined from" lists in its
// constituent DrmShimBuffer.
// The result will be a list of all the constituent transforms that should align
// with the original layer list.
void DrmShimBuffer::AddSourceFBsToList(DrmShimSortedTransformVector& list,
                                       DrmShimTransform& thisTransform,
                                       uint32_t sources) {
  char str[HWCVAL_DEFAULT_STRLEN];
  char strbuf2[HWCVAL_DEFAULT_STRLEN];

  sources |= 1 << static_cast<int>(GetSource());

  HWCLOGV_COND(eLogCombinedTransform,
               "DrmShimBuffer::AddSourceFBsToList Enter: transform@%p, buf@%p, "
               "sources 0x%x",
               &thisTransform, thisTransform.GetBuf().get(), sources);

  if (mCombinedFrom.size() > 0) {
    HWCLOGV_COND(eLogCombinedTransform, "%s transform@%p adding srcs %s:",
                 IdStr(str), &thisTransform,
                 DrmShimTransform::SourcesStr(sources, strbuf2));
    for (uint32_t i = 0; i < mCombinedFrom.size(); ++i) {
      DrmShimTransform& transform = mCombinedFrom.editItemAt(i);
      DrmShimTransform combinedTransform(transform, thisTransform,
                                         eLogCombinedTransform);
      transform.GetBuf()->AddSourceFBsToList(list, combinedTransform, sources);
    }
    HWCLOGV_COND(eLogCombinedTransform,
                 "DrmShimBuffer::AddSourceFBsToList Exit: transform@%p, "
                 "buf@%p, sources 0x%x",
                 &thisTransform, thisTransform.GetBuf().get(), sources);
    return;
  }

  thisTransform.SetSources(sources);
  ssize_t pos = list.add(thisTransform);

  // End of real function, start of debug info
  if (HWCCOND(eLogCombinedTransform)) {
    DrmShimSortedTransformVector tmp(list);
    DrmShimTransform& copyTransform = list.editItemAt(pos);
    uint32_t s = 0;

    HWCLOGV("  Adding original %s transform@%p->%p pos %d/%d srcs: %s.",
            IdStr(str), &thisTransform, &copyTransform, pos, list.size(),
            DrmShimTransform::SourcesStr(sources, strbuf2));

    copyTransform.Log(ANDROID_LOG_VERBOSE, "  Added FB");

    for (uint32_t i = 0; i < list.size(); ++i) {
      DrmShimTransform& tr = list.editItemAt(i);
      if (tr.GetBuf() != NULL) {
        HWCLOGI("%d %s", i, tr.GetBuf()->IdStr(str));
      } else {
        HWCLOGI("%d buf@0", i);
      }

      if (tr.GetBuf() == thisTransform.GetBuf()) {
        continue;
      }

      if (s >= tmp.size()) {
        HWCERROR(eCheckInternalError,
                 "TRANSFORM MISMATCH: TOO MANY TRANSFORMS IN RESULT");
        continue;
      }

      DrmShimTransform& sr = tmp.editItemAt(s);
      if (tr.GetBuf() != sr.GetBuf()) {
        HWCERROR(eCheckInternalError,
                 "TRANSFORM MISMATCH: RESULT CONTAINS buf@%p NOT IN SOURCE",
                 tr.GetBuf().get());
        continue;
      }

      s++;
    }  // End for list.size()
    if (s + 1 < tmp.size()) {
      HWCERROR(eCheckInternalError,
               "TRANSFORM MISMATCH: NOT ALL SOURCES COPIED");
    }
  }

  HWCLOGV_COND(eLogCombinedTransform,
               "DrmShimBuffer::AddSourceFBsToList Exit: transform@%p, buf@%p, "
               "sources 0x%x",
               &thisTransform, thisTransform.GetBuf().get(), sources);
}

void DrmShimBuffer::SetAllCombinedFrom(
    const DrmShimTransformVector& combinedFrom) {
  HWCLOGD("SetAllCombinedFrom: buf@%p handle %p combined from %d transforms",
          this, mHandle, combinedFrom.size());
  mCombinedFrom = combinedFrom;
}

const DrmShimTransformVector& DrmShimBuffer::GetAllCombinedFrom() {
  return mCombinedFrom;
}

uint32_t DrmShimBuffer::NumCombinedFrom() const {
  return mCombinedFrom.size();
}

void DrmShimBuffer::Unassociate() {
  mCombinedFrom.clear();

  mFbtDisplay = -1;
}

DrmShimBuffer* DrmShimBuffer::SetLastHwcFrame(Hwcval::FrameNums fn,
                                              bool isOnSet) {
  mLastHwcFrame = fn;

  if (isOnSet) {
    mLastOnSetFrame = fn;
  }

  return this;
}

// This function determines whether a buffer is still 'current', i.e. the
// content is unchanged.
// HWC (from HWC 2.0 changes onwards) will do this by looking to see when the
// reference count of the buffer
// goes to zero.
//
// That is too complex for us right now. So we are looking to see if the buffer
// was used in the last frame
// on ANY of the displays.
//
// I suspect that this will need some work for HWC 2.0 to get it fully working.
// For example if one of the
// displays is turned off, it may appear that the buffer is still current.
bool DrmShimBuffer::IsCurrent(Hwcval::FrameNums fn) {
  for (uint32_t d = 0; d < HWCVAL_MAX_CRTCS; ++d) {
    if (fn[d] == HWCVAL_UNDEFINED_FRAME_NUMBER) {
      continue;
    }

    if ((mLastHwcFrame[d] + 1) >= fn[d]) {
      return true;
    }
  }

  return false;
}

const char* DrmShimBuffer::GetHwcFrameStr(char* str, uint32_t len) {
  if (HWCVAL_MAX_CRTCS >= 3) {
    snprintf(str, len, "frame:%d.%d.%d", mLastHwcFrame[0], mLastHwcFrame[1],
             mLastHwcFrame[2]);
  }

  return str;
}

void DrmShimBuffer::SetToBeCompared(bool toBeCompared) {
  mToBeCompared = toBeCompared;
}

bool DrmShimBuffer::IsToBeComparedOnce() {
  return (android_atomic_swap(0, &mToBeCompared) != 0);
}

bool DrmShimBuffer::IsToBeCompared() {
  return mToBeCompared;
}

// Set local copy of the buffer contents
// so we can do comparisons after the original buffer has been deallocated
void DrmShimBuffer::SetBufCopy(android::sp<android::GraphicBuffer>& buf) {
  if ((mBufCpy.get() == 0) && buf.get()) {
    ++sNumBufCopies;
    if (sNumBufCopies > 10) {
      HWCLOGI("%d copies of buffers stored for transparency filter detection",
              sNumBufCopies);
    }
  } else if (mBufCpy.get() && (buf.get() == 0)) {
    --sNumBufCopies;
  }

  mBufCpy = buf;
}

android::sp<android::GraphicBuffer> DrmShimBuffer::GetBufCopy() {
  return mBufCpy;
}

bool DrmShimBuffer::HasBufCopy() {
  return (mBufCpy != 0);
}

DrmShimBuffer* DrmShimBuffer::IncAppearanceCount() {
  ++mAppearanceCount;
  return this;
}

void DrmShimBuffer::ResetAppearanceCount() {
  mAppearanceCount = 0;
}

uint32_t DrmShimBuffer::GetAppearanceCount() {
  return mAppearanceCount;
}

void DrmShimBuffer::SetRef(android::sp<android::GraphicBuffer>& buf) {
  mRef = buf;
}

uint32_t DrmShimBuffer::GetBpp() {
  char strbuf[HWCVAL_DEFAULT_STRLEN];
  HWCLOGD_COND(eLogFlicker, "%s format %s", IdStr(strbuf), StrBufFormat());

  switch (GetDrmFormat()) {
    case DRM_FORMAT_RGB332:
    case DRM_FORMAT_BGR233:
      return 8;

    case DRM_FORMAT_XRGB4444:
    case DRM_FORMAT_XBGR4444:
    case DRM_FORMAT_RGBX4444:
    case DRM_FORMAT_BGRX4444:
    case DRM_FORMAT_ARGB4444:
    case DRM_FORMAT_ABGR4444:
    case DRM_FORMAT_RGBA4444:
    case DRM_FORMAT_BGRA4444:
    case DRM_FORMAT_XRGB1555:
    case DRM_FORMAT_XBGR1555:
    case DRM_FORMAT_RGBX5551:
    case DRM_FORMAT_BGRX5551:
    case DRM_FORMAT_ARGB1555:
    case DRM_FORMAT_ABGR1555:
    case DRM_FORMAT_RGBA5551:
    case DRM_FORMAT_BGRA5551:
    case DRM_FORMAT_RGB565:
    case DRM_FORMAT_BGR565:

    case DRM_FORMAT_YUYV:
      return 16;

    case DRM_FORMAT_RGB888:
    case DRM_FORMAT_BGR888:
      return 24;

    default:
      return 32;
  }
}

// Return a string specifying the buffer format.
const char* DrmShimBuffer::StrBufFormat() {
  *((uint32_t*)&mStrFormat) = GetDrmFormat();
  mStrFormat[4] = '\0';
  return mStrFormat;
}

// Return the identification string of the DrmShimBuffer
// This logs out all the interesting identification info including gralloc
// handle, buffer objects and framebuffer IDs.
char* DrmShimBuffer::IdStr(char* str, uint32_t len) const {
  uint32_t n = snprintf(str, len, "buf@%p handle %p " BUFIDSTR " 0x%x ", this,
                        mHandle, GetGlobalId());
  if (n >= len)
    return str;

  if (HWCCOND(eLogBuffer)) {
    for (uint32_t i = 0; i < mBos.size(); ++i) {
      android::sp<HwcTestBufferObject> bo = mBos.itemAt(i);
      n += bo->FullIdStr(str + n, len - n);
      if (n >= len)
        return str;

      n += snprintf(str + n, len - n, " ");
      if (n >= len)
        return str;

      // If the bo's reverse pointer to the buffer is wrong, log out what it
      // actually points to.
      android::sp<DrmShimBuffer> boBuf = bo->mBuf.promote();
      if (boBuf.get() != this) {
        snprintf(str + n, len - n, "(!!!buf=%p) ", boBuf.get());
      }
    }
  }

  for (uint32_t i = 0; i < mFbIds.size(); ++i) {
    uint32_t fbId = mFbIds.keyAt(i);

    if (i == 0) {
      n += snprintf(str + n, len - n, "FB %d", fbId);
    } else {
      n += snprintf(str + n, len - n, ",%d", fbId);
    }

    if (n >= len)
      return str;
  }

  if (mDsId > 0) {
    n += snprintf(str + n, len - n, "DS %" PRIi64, mDsId);
    if (n >= len)
      return str;
  }

  if (HWCVAL_MAX_CRTCS >= 3) {
    n += snprintf(str + n, len - n, " (last seen %s)",
                  android::String8(mLastOnSetFrame).string());
  }

  return str;
}

// Return the buffer source as a string
const char* DrmShimBuffer::GetSourceName() {
  switch (mBufferSource) {
    case Hwcval::BufferSourceType::Input:
      return "Input";
    case Hwcval::BufferSourceType::PartitionedComposer:
      return "PartitionedComposer";
    case Hwcval::BufferSourceType::Writeback:
      return "Writeback";
    default:
      return "Unknown source";
  }
}

// Report buffer status for debug purposes
void DrmShimBuffer::ReportStatus(int priority, const char* str) {
  char strbuf[HWCVAL_DEFAULT_STRLEN];

  // For efficiency, filter the logging at this point
  if (HwcGetTestConfig()->IsLevelEnabled(priority)) {
    HWCLOG(priority, "%s: %s %s %s", str, IdStr(strbuf), GetSourceName(),
           mBlanking ? "+Blanking" : "-Blanking");
    HWCLOG(priority, "  Size %dx%d Alloc %dx%d DrmFormat %x Usage %x",
#ifdef HWCVAL_FB_BUFFERINFO_FORMAT
           mDetails.width, mDetails.height, mDetails.allocWidth,
           mDetails.allocHeight, mDetails.fb_format, mDetails.usage);
#else
           mDetails.width, mDetails.height, mDetails.allocWidth,
           mDetails.allocHeight, mDetails.drmformat, mDetails.usage);
#endif

    if (mCombinedFrom.size() > 0) {
      char linebuf[200];
      char entrybuf[100];
      bool commaNeeded = false;
      linebuf[0] = '\0';

      for (uint32_t i = 0; i < mCombinedFrom.size(); ++i) {
        // Just for safety, never likely to happen
        if (strlen(linebuf) > (sizeof(linebuf) - sizeof(entrybuf))) {
          strcat(linebuf, "...");
          break;
        }

        DrmShimTransform& transform = mCombinedFrom.editItemAt(i);

        char str[HWCVAL_DEFAULT_STRLEN];
        sprintf(entrybuf, "%s", transform.GetBuf()->IdStr(str));

        if (commaNeeded) {
          strcat(linebuf, ", ");
        }
        strcat(linebuf, entrybuf);
        commaNeeded = true;
      }
      HWCLOG(priority, "  CombinedFrom: %s", linebuf);
    }
  }
}

// Debugging function
void DrmShimBuffer::DbgCheckNoReferenceTo(DrmShimBuffer* buf) const {
  char strbuf[HWCVAL_DEFAULT_STRLEN];

  for (uint32_t cfix = 0; cfix < NumCombinedFrom(); ++cfix) {
    const DrmShimTransform& from = mCombinedFrom.itemAt(cfix);
    if (from.GetConstBuf() == buf) {
      HWCERROR(eCheckInternalError,
               "Deleting %s which is referenced in combinedFrom %s",
               buf->IdStr(strbuf), from.GetConstBuf()->IdStr(strbuf));
    }
  }
}

// Determine if the buffer is transparent.
// The transparency state is cached so that the determination is done at most
// once for each buffer
// This checking should only be done for buffers that are in front of a NV12
// layer and have remained in the layer list
// for a long time.
bool DrmShimBuffer::IsBufferTransparent(const hwc_rect_t& rect) {
  uint32_t logLevel = ANDROID_LOG_DEBUG;
  if (mTransparentFromHarness) {
    logLevel = ANDROID_LOG_WARN;
  }

  HWCLOG(
      logLevel,
      "DrmShimBuffer::IsBufferTransparent entry buf@%p handle %p %s rect(%d, "
      "%d, %d, %d)",
      this, mHandle,
      (mBufferContent == BufferContentType::ContentNull)
          ? "Null"
          : ((mBufferContent == BufferContentType::ContentNotNull) ? "Not Null"
                                                                   : ""),
      rect.left, rect.top, rect.right, rect.bottom);

  if (mBufferContent == BufferContentType::ContentNotTested) {
    hw_module_t const* module;

    int err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
    ALOG_ASSERT(!err);

    struct gralloc_module_t* pGralloc = (struct gralloc_module_t*)module;
    ALOG_ASSERT(pGralloc);

    if (HasBufCopy()) {
      // We MUST query the buffer copy for details rather than just using
      // mDetails,
      // because it will have a different pitch to the original buffer seeing as
      // we have
      // requested a copy in linear memory.
      Hwcval::buffer_details_t bi;

      HWCCHECK(eCheckGrallocDetails);
      if (GetBufferInfo(mBufCpy->handle, &bi)) {
        HWCERROR(eCheckGrallocDetails,
                 "DrmShimBuffer::IsBufferTransparent can't get info for buf@%p "
                 "handle %p copy %p",
                 this, mHandle, mBufCpy->handle);

        return false;
      }

      mBufferContent = IsBufferTransparent(pGralloc, bi, mBufCpy->handle, rect)
                           ? BufferContentType::ContentNull
                           : BufferContentType::ContentNotNull;
    } else {
      mBufferContent = BufferContentType::ContentNotNull;
    }
  }

  HWCLOG(
      logLevel, "DrmShimBuffer::IsBufferTransparent exit buf@%p handle%p %s",
      this, mHandle,
      (mBufferContent == BufferContentType::ContentNull)
          ? "Null"
          : ((mBufferContent == BufferContentType::ContentNotNull) ? "Not Null"
                                                                   : ""));

  return (mBufferContent == BufferContentType::ContentNull);
}

// Static function to read a buffer and find out if it is empty within a certain
// rectangle.
bool DrmShimBuffer::IsBufferTransparent(struct gralloc_module_t* gralloc,
                                        Hwcval::buffer_details_t& bi,
                                        buffer_handle_t handle,
                                        const hwc_rect_t& rect) {
  if ((bi.height < rect.bottom) || (bi.width < rect.right)) {
    HWCLOGD("video (%d, %d, %d, %d) overlay %dx%d", rect.left, rect.top,
            rect.right, rect.bottom, bi.width, bi.height);
    HWCLOGD(
        "DrmShimBuffer::IsBufferTransparent: Not fully covering video: handle "
        "%p",
        handle);
    return false;
  }

  uint8_t* data = 0;
  int err = gralloc->lock(gralloc, handle, GRALLOC_USAGE_SW_READ_OFTEN, 0, 0, 0,
                          0, (void**)&data);

  if (err || (data == 0)) {
    HWCLOGW(
        "DrmShimBuffer::IsBufferTransparent: Gralloc lock of real buffer "
        "handle %p failed with err=%d",
        handle, err);
    return false;
  }

  uint32_t lineStrideBytes = bi.pitch;
  uint32_t bytesPerPixel = bi.pitch / bi.width;  // could be wrong
  uint32_t lineWidthBytes = (rect.right - rect.left) * bytesPerPixel;

  bool ret = true;
  uint32_t n = 0;

  for (int i = rect.top; i < rect.bottom; ++i) {
    uint8_t* pbyte = (data + i * lineStrideBytes + bytesPerPixel * rect.left);
    uint64_t* p = (uint64_t*)pbyte;

    uint32_t j;
    for (j = 0; j < ((lineWidthBytes - 7) / 8); ++j) {
      ++n;
      if (p[j] != 0) {
        HWCLOGD(
            "DrmShimBuffer::IsBufferTransparent: Not null line %d doubleword "
            "%d 0x%llx",
            i, j, p[j]);
        ret = false;
        break;
      }
    }

    j *= 8;
    for (; j < lineWidthBytes; ++j) {
      if (pbyte[j] != 0) {
        HWCLOGD("DrmShimBuffer::IsBufferTransparent: Not null line %d byte %d",
                i, j);
        ret = false;
        break;
      }
    }

    if (!ret) {
      break;
    }
  }

  HWCLOGD(
      "DrmShimBuffer::IsBufferTransparent tested %d doublewords. "
      "linewidthbytes=%d (%d, %d, %d, %d)",
      n, lineWidthBytes, rect.left, rect.right, rect.top, rect.bottom);

  gralloc->unlock(gralloc, handle);
  return ret;
}

void DrmShimBuffer::SetTransparentFromHarness() {
  mTransparentFromHarness = true;
}

bool DrmShimBuffer::IsActuallyTransparent() {
  return mTransparentFromHarness;
}

void DrmShimBuffer::ReportCompositionMismatch(
    uint32_t lineWidthBytes, uint32_t lineStrideCpy, uint32_t lineStrideRef,
    double SSIMIndex, unsigned char* cpyData, unsigned char* refData) {
  ++mCompMismatchCount;

  // Do some stats
  uint32_t numMismatchBytes = 0;
  uint64_t sumOfSquares = 0;
  int mismatchLine = -1;

  for (int i = 0; i < mDetails.height; ++i) {
    uint8_t* realDataLine = cpyData + (i * lineStrideCpy);
    uint8_t* refDataLine = refData + (i * lineStrideRef);

    uint64_t lineSumOfSquares = 0;

    for (uint32_t j = 0; j < lineWidthBytes; ++j) {
      if (realDataLine[j] != refDataLine[j]) {
        if (mismatchLine < 0) {
          mismatchLine = i;
        }

        ++numMismatchBytes;
        int diff = ((int)realDataLine[j]) - ((int)refDataLine[j]);
        lineSumOfSquares += diff * diff;
      }
    }

    sumOfSquares += lineSumOfSquares;
  }

  uint32_t numBytes = mDetails.height * lineWidthBytes;
  double meanSquares = ((double)sumOfSquares) / numBytes;
  double rms = sqrt(meanSquares);

  double percentageMismatch = 100.0 * ((double)numMismatchBytes) / numBytes;

  HWCERROR(IsFbt() ? eCheckSfCompMatchesRef : eCheckHwcCompMatchesRef,
           "CompareWithRef: Composition mismatch %d with real buffer handle %p "
           "from frame:%d at line %d",
           mCompMismatchCount, mHandle, mLastHwcFrame, mismatchLine);
  // NB %% does not work
  HWCLOGE(
      "  -- %2.6f%%%% of bytes mismatch; RMS = %3.6f; SSIM index = %f "
      "(frame:%d)",
      percentageMismatch, rms, SSIMIndex, mLastHwcFrame);

  // If we haven't already made too many files, dump the SF and reference data
  // to TGA files so we can examine them later
  if ((mCompMismatchCount * 2) < MAX_SF_MISMATCH_DUMP_FILES) {
    HwcTestDumpGrallocBufferToDisk("real", mCompMismatchCount, mBufCpy->handle,
                                   DUMP_BUFFER_TO_TGA);
    HwcTestDumpGrallocBufferToDisk("ref", mCompMismatchCount, mRef->handle,
                                   DUMP_BUFFER_TO_TGA);
  }
}

// Compare the contents of the buffer with the reference composition using SSIM
// (Structural Similarity algorithm).
bool DrmShimBuffer::CompareWithRef(bool useAlpha, hwc_rect_t* rectToCompare) {
  char strbuf[HWCVAL_DEFAULT_STRLEN];

  if (mRef.get() == 0) {
    HWCERROR(eCheckInternalError, "CompareWithRef: %s NO REF!!", IdStr(strbuf));
    return false;
  } else {
    HWCLOGD("CompareWithRef: %s mem@%p compared with ref handle %p",
            IdStr(strbuf), mRef->handle);
  }

  Hwcval::buffer_details_t cpyBi;
  GetBufferInfo(mBufCpy->handle, &cpyBi);
  Hwcval::buffer_details_t refBi;
  GetBufferInfo(mRef->handle, &refBi);

  unsigned char* cpyData;
  unsigned char* refData;
  uint32_t ret = mBufCpy->lock(GRALLOC_USAGE_SW_READ_MASK, (void**)&cpyData);

  if (ret || (cpyData == 0)) {
    HWCLOGW("CompareWithRef: Failed to lock cpy buffer");
    return false;
  }

  ret = mRef->lock(GRALLOC_USAGE_SW_READ_MASK, (void**)&refData);

  if (ret || (refData == 0)) {
    HWCLOGW("CompareWithRef: Failed to lock ref buffer");
    mBufCpy->unlock();
    return false;
  }

  uint32_t left = 0;
  uint32_t top = 0;
  uint32_t right = cpyBi.width;
  uint32_t bottom = cpyBi.height;

  if (rectToCompare) {
    left = rectToCompare->left;
    top = rectToCompare->top;
    right = rectToCompare->right;
    bottom = rectToCompare->bottom;
  }

  uint32_t width = right - left;
  uint32_t height = bottom - top;

  // Compare data line by line
  // uint32_t lineStrideBytes = cpyBi.pitch;
  uint32_t bytesPerPixel = cpyBi.pitch / cpyBi.width;
  uint32_t lineWidthBytes = width * bytesPerPixel;
  HWCLOGD(
      "CompareWithRef: Comparing real %p ref %p (%d, %d) %dx%d Pitch %d Bytes "
      "Per Pixel %d",
      cpyData, refData, left, top, width, height, cpyBi.pitch, bytesPerPixel);

  bool same = true;
  for (uint32_t i = 0; i < height; ++i) {
    uint8_t* realDataLine =
        cpyData + ((i + top) * cpyBi.pitch) + (left * bytesPerPixel);
    uint8_t* refDataLine =
        refData + ((i + top) * refBi.pitch) + (left * bytesPerPixel);

    if (memcmp(realDataLine, refDataLine, lineWidthBytes) != 0) {
      same = false;
      break;
    }
  }

  if (!same) {
    // SSIM comparison algorithm

    double SSIMIndex = 0;
    dssim_info* dinf = new dssim_info();

    // load image content in the row pointers

    unsigned char* BufRowPointers[height];
    unsigned char* RefRowPointers[height];

    for (uint32_t i = 0; i < height; ++i) {
      // assign pointer for row
      BufRowPointers[i] =
          cpyData + ((i + top) * cpyBi.pitch) + (left * bytesPerPixel);
    }

    for (uint32_t i = 0; i < height; ++i) {
      // assign pointer for row
      RefRowPointers[i] =
          refData + ((i + top) * refBi.pitch) + (left * bytesPerPixel);
    }

    // set up timing information

    int64_t startTime = ns2ms(systemTime(SYSTEM_TIME_MONOTONIC));

    // SSIM preliminary calculations

    const int blur_type = ebtLinear;  // TODO - remove hard-coded option
    bool hasAlpha = (GetFormat() == HAL_PIXEL_FORMAT_RGBA_8888) && useAlpha;

    DoSSIMCalculations(dinf, (dssim_rgba**)BufRowPointers,
                       (dssim_rgba**)RefRowPointers, width, height, blur_type,
                       hasAlpha);

    // calculate SSIM index averaged on channels

    double channelResult[CHANS];
    for (int ch = 0; ch < CHANS; ++ch) {
      channelResult[ch] = GetSSIMIndex(&dinf->chan[ch]);
    }

    HWCLOGD("SSIM indices per channel: %f %f %f", channelResult[0],
            channelResult[1], channelResult[2]);

    for (int ch = 0; ch < CHANS; ch++) {
      SSIMIndex += channelResult[ch];
    }
    SSIMIndex /= double(CHANS);

    // retrieve time information

    HWCLOGD("%s SSIM index = %.6f", __FUNCTION__, SSIMIndex);
    HWCLOGD("%s SSIM algorithm execution time in milliseconds: %llu",
            __FUNCTION__,
            (ns2ms(systemTime(SYSTEM_TIME_MONOTONIC)) - startTime));

    // deallocations

    delete (dinf);

    // END SSIM comparison algorithm

    if (SSIMIndex < HWCVAL_SSIM_ACCEPTANCE_LEVEL) {
      ReportCompositionMismatch(lineWidthBytes, cpyBi.pitch, refBi.pitch,
                                SSIMIndex, cpyData, refData);
    } else {
      HWCLOGI(
          "CompareWithRef: %s: Comparison passed with SSIM Index = %.6f "
          "(frame:%d)",
          IdStr(strbuf), SSIMIndex, mLastHwcFrame);
    }
  } else {
    HWCLOGI("CompareWithRef: %s comparison pass (identical)", IdStr(strbuf));
  }

  // This matches the potential error in ReportCompositionMismatch()
  HWCCHECK(IsFbt() ? eCheckSfCompMatchesRef : eCheckHwcCompMatchesRef);

  mBufCpy->unlock();
  mRef->unlock();

  FreeBufCopies();

  return same;
}

bool DrmShimBuffer::HasRef() {
  return (mRef.get() != 0);
}


const char* DrmShimBuffer::ValidityStr(Hwcval::ValidityType valid) {
  switch (valid) {
    case ValidityType::Invalid: {
      return "Invalid";
    }
    case ValidityType::InvalidWithinTimeout: {
      return "Invalid within timeout";
    }
    case ValidityType::Invalidating: {
      return "Invalidating";
    }
    case ValidityType::ValidUntilModeChange: {
      return "Valid until mode change";
    }
    case ValidityType::Valid: {
      return "Valid";
    }
    case ValidityType::Indeterminate: {
      return "Indeterminate";
    }
    default: { return "Unknown"; }
  }
}

uint32_t DrmShimBuffer::GetAuxOffset() {
#ifdef HWCVAL_ENABLE_RENDER_COMPRESSION
  return mDetails.rc.aux_offset;
#else
  return 0;
#endif
}

uint32_t DrmShimBuffer::GetAuxPitch() {
#ifdef HWCVAL_ENABLE_RENDER_COMPRESSION
  return mDetails.rc.aux_pitch;
#else
  return 0;
#endif
}

bool DrmShimBuffer::FormatHasPixelAlpha(uint32_t format) {
  return HasAlpha(format);
}

bool DrmShimBuffer::FormatHasPixelAlpha() {
  return FormatHasPixelAlpha(GetFormat());
}
