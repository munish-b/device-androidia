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
#include "HwcvalContent.h"

#include "hardware/hwcomposer_defs.h"
#include <utils/RefBase.h>
#include "DrmShimBuffer.h"

#include <vector>

namespace Hwcval
{
    ValRegion::ValRegion(const hwc_region_t& region)
    {
        mRects.resize(region.numRects);

        for (uint32_t i=0; i<region.numRects; ++i)
        {
            // Could use emplace if we werent worried about MCG 5.1 compatibility
            mRects.push_back(region.rects[i]);
        }
    }

    ValRegion::ValRegion()
    {
    }

    void ValRegion::GetHwcRects(hwc_region_t& region, hwc_rect_t*& pRect, uint32_t& rectsRemaining) const
    {
        region.numRects = mRects.size();
        region.rects = pRect;

        ALOG_ASSERT(mRects.size() < rectsRemaining);

        for (uint32_t i=0; i<mRects.size(); ++i)
        {
            pRect[i] = mRects[i];
        }

        pRect += mRects.size();
        rectsRemaining -= mRects.size();
    }

    uint32_t ValRegion::NumRects() const
    {
        return mRects.size();
    }

    hwc_rect_t ValRegion::GetBounds(const hwc_rect_t& displayFrame) const
    {
        // Find bounding box of visible regions
        if (mRects.size() > 0)
        {
            hwc_rect_t bounds(mRects[0]);
            HWCLOGD_COND(eLogVisibleRegions, "VR0: (%d, %d, %d, %d)", bounds.left, bounds.top, bounds.right, bounds.bottom);

            for (uint32_t i=1; i<mRects.size(); ++i)
            {
                const hwc_rect_t& r = mRects[i];
                HWCLOGD_COND(eLogVisibleRegions, "VR%d: (%d, %d, %d, %d)", i, r.left, r.top, r.right, r.bottom);

                if (r.left < bounds.left)
                {
                    bounds.left = r.left;
                }

                if (r.top < bounds.top)
                {
                    bounds.top = r.top;
                }

                if (r.right > bounds.right)
                {
                    bounds.right = r.right;
                }

                if (r.bottom > bounds.bottom)
                {
                    bounds.bottom = r.bottom;
                }
            }

            HWCLOGD_COND(eLogVisibleRegions, "Visible Regions: Bounds (%d, %d, %d, %d)", bounds.left, bounds.top, bounds.right, bounds.bottom);

            return bounds;
        }
        else
        {
            return displayFrame;
        }
    }

    LayerList::LayerList(uint32_t numLayers)
    {
        mLayers.reserve(numLayers);
    }

    void LayerList::Add(const ValLayer& layer)
    {
        // C++11: Avoid temporaries by use of emplace_back?
        mLayers.push_back(layer);
    }

    void LayerList::VideoFlags::Log(const char* str, uint32_t d, uint32_t hwcFrame)
    {
        HWCLOGD("%s: D%d frame:%d Video flags@%p: singleFS %d FS %s PS %d",
            str, d, hwcFrame,
            this,
            mSingleFullScreenVideo,
            TriStateStr(mFullScreenVideo),
            mPartScreenVideo);
    }


    // Returns true if layer stack contains at least one video layer
    bool LayerList::IsVideo()
    {
        for (uint32_t i = 0; i < mNumLayers; ++i)
        {
            android::sp<DrmShimBuffer> buf = mLayers[i].GetBuf();
            if (buf.get() && buf->IsVideoFormat())
            {
                return true;
            }
        }

        return false;
    }

    buffer_handle_t ValLayer::GetHandle() const
    {
        if (mBuf.get())
        {
            return mBuf->GetHandle();
        }
        else
        {
            return 0;
        }
    }
}

