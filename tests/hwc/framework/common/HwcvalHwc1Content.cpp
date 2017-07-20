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

#include "HwcvalHwc1Content.h"
#include "DrmShimBuffer.h"
#include "HwcTestState.h"

#include <hardware/hwcomposer2.h>

Hwcval::CompositionType Hwcval::Hwc1CompositionTypeToHwcval(uint32_t compositionType)
{
    switch (compositionType)
    {
        case HWC_FRAMEBUFFER:
            return Hwcval::CompositionType::SF;
        case HWC_OVERLAY:
            return Hwcval::CompositionType::HWC;
        case HWC_FRAMEBUFFER_TARGET:
            return Hwcval::CompositionType::TGT;
        default:
            return Hwcval::CompositionType::UNKNOWN;
    }
}

Hwcval::BlendingType Hwcval::Hwc1BlendingTypeToHwcval(uint32_t blendingType)
{
    switch (blendingType)
    {
        case HWC_BLENDING_NONE:
            return Hwcval::BlendingType::NONE;
        case HWC_BLENDING_PREMULT:
            return Hwcval::BlendingType::PREMULTIPLIED;
        case HWC_BLENDING_COVERAGE:
            return Hwcval::BlendingType::COVERAGE;
        default:
            return Hwcval::BlendingType::INVALID;
    }
}

Hwcval::Hwc1Layer::Hwc1Layer(const hwcval_layer_t *sfLayer,
                             android::sp<DrmShimBuffer> &buf) {
    mCompositionType = Hwc1CompositionTypeToHwcval(sfLayer->compositionType);
    mHints = sfLayer->hints;
    mFlags = sfLayer->flags;
    mBuf = buf;

    // HWC1 transforms have the same values as internal Hwcval transforms
    mTransform = sfLayer->transform;

    mBlending = Hwc1BlendingTypeToHwcval(sfLayer->blending);
    mSourceCropf = sfLayer->sourceCropf;
    mDisplayFrame = sfLayer->displayFrame;
    mPlaneAlpha = float(sfLayer->planeAlpha) / HWCVAL_ALPHA_FLOAT_TO_INT;

    if (buf.get())
    {
        // Copy the visible rects to separate area and provide link from layer
        mVisibleRegionScreen = ValRegion(sfLayer->visibleRegionScreen);
    }
}

Hwcval::Hwc1LayerList::Hwc1LayerList(
    const hwcval_display_contents_t *sfDisplay) {
    mRetireFenceFd = 0; // Correct value won't be known until exit of OnSet

    if (sfDisplay)
    {
        mOutbuf = sfDisplay->outbuf;                // This will change when we do virtual displays
        // mOutbufAcquireFenceFd = sfDisplay->outbufAcquireFenceFd;
        // mFlags = sfDisplay->flags;
        mNumLayers = sfDisplay->numHwLayers;
    }
    else
    {
        mOutbufAcquireFenceFd = 0;
        mFlags = 0;
        mNumLayers = 0;
    }
}

uint32_t Hwcval::HwcvalBlendingTypeToHwc1(Hwcval::BlendingType blendingType)
{
    switch (blendingType)
    {
        case Hwcval::BlendingType::NONE:
            return HWC_BLENDING_NONE;
        case Hwcval::BlendingType::PREMULTIPLIED:
            return HWC_BLENDING_PREMULT;
        case Hwcval::BlendingType::COVERAGE:
            return HWC_BLENDING_COVERAGE;
        default:
            return HWC_BLENDING_NONE;
    }
}

// Convert internal layer format back to hwcval_layer_t.
void Hwcval::HwcvalLayerToHwc1(const char *str, uint32_t ix,
                               hwcval_layer_t &out, Hwcval::ValLayer &in,
                               hwc_rect_t *pRect, uint32_t &rectsRemaining) {
    const hwc_frect_t& sourceCropf = in.GetSourceCrop();
    const hwc_rect_t displayFrame = in.GetDisplayFrame();

    HWCLOGV("%s %d handle %p src (%f,%f,%f,%f) dst (%d,%d,%d,%d) alpha %d",
            str, ix, in.GetHandle(),
            (double) sourceCropf.left, (double) sourceCropf.top, (double) sourceCropf.right, (double) sourceCropf.bottom,
            displayFrame.left, displayFrame.top, displayFrame.right, displayFrame.bottom, in.GetPlaneAlpha());
    out.handle = in.GetHandle();
    out.sourceCropf = sourceCropf;
    out.displayFrame = displayFrame;
    out.transform = in.GetTransformId();
    out.blending = Hwcval::HwcvalBlendingTypeToHwc1(in.GetBlendingType());

    // Convert plane alpha from internal (floating point) form to the integer form expected by the composer.
    //
    // I'm using 0.25 rather than the usual 0.5 because we know that the original data was in integer form
    // and I don't want to end up incrementing the integer result.
    out.planeAlpha = (in.GetPlaneAlpha() * HWCVAL_ALPHA_FLOAT_TO_INT) + 0.25;

    in.GetVisibleRegion().GetHwcRects(out.visibleRegionScreen, pRect, rectsRemaining);

    // Composition type is left to the caller
}




