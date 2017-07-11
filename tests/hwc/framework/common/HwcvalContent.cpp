/****************************************************************************

Copyright (c) Intel Corporation (2014).

DISCLAIMER OF WARRANTY
NEITHER INTEL NOR ITS SUPPLIERS MAKE ANY REPRESENTATION OR WARRANTY OR
CONDITION OF ANY KIND WHETHER EXPRESS OR IMPLIED (EITHER IN FACT OR BY
OPERATION OF LAW) WITH RESPECT TO THE SOURCE CODE.  INTEL AND ITS SUPPLIERS
EXPRESSLY DISCLAIM ALL WARRANTIES OR CONDITIONS OF MERCHANTABILITY OR
FITNESS FOR A PARTICULAR PURPOSE.  INTEL AND ITS SUPPLIERS DO NOT WARRANT
THAT THE SOURCE CODE IS ERROR-FREE OR THAT OPERATION OF THE SOURCE CODE WILL
BE SECURE OR UNINTERRUPTED AND HEREBY DISCLAIM ANY AND ALL LIABILITY ON
ACCOUNT THEREOF.  THERE IS ALSO NO IMPLIED WARRANTY OF NON-INFRINGEMENT.
SOURCE CODE IS LICENSED TO LICENSEE ON AN "AS IS" BASIS AND NEITHER INTEL
NOR ITS SUPPLIERS WILL PROVIDE ANY SUPPORT, ASSISTANCE, INSTALLATION,
TRAINING OR OTHER SERVICES.  INTEL AND ITS SUPPLIERS WILL NOT PROVIDE ANY
UPDATES, ENHANCEMENTS OR EXTENSIONS.

File Name:      HwcvalContent.cpp

Description:    Class implementation for HWC validation internal representation
                of layer list.

Environment:

Notes:

****************************************************************************/
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

