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
#ifndef __HwcvalHwc1Content_h__
#define __HwcvalHwc1Content_h__

#include "Hwcval.h"
#include "HwcvalContent.h"

namespace Hwcval
{
    // Conversion functions
    Hwcval::CompositionType Hwc1CompositionTypeToHwcval(uint32_t compositionType);
    Hwcval::BlendingType Hwc1BlendingTypeToHwcval(uint32_t blendingType);
    uint32_t HwcvalBlendingTypeToHwc1(Hwcval::BlendingType blendingType);
    void HwcvalLayerToHwc1(const char *str, uint32_t ix, hwcval_layer_t &out,
                           Hwcval::ValLayer &in, hwc_rect_t *pRect,
                           uint32_t &rectsRemaining);

    class Hwc1Layer : public ValLayer
    {
    public:
      Hwc1Layer(const hwcval_layer_t *sfLayer, android::sp<DrmShimBuffer> &buf);
    };

    // Hwc1Layer should add only methods
    //static_assert(sizeof(Hwc1Layer) == sizeof(ValLayer));


    /*
     * Description of the contents to output on a display.
     *
     * This is the top-level structure passed to the prepare and set calls to
     * negotiate and commit the composition of a display image.
     */
    class Hwc1LayerList : public LayerList
    {
    public:
        // Constructor
        // Only copies the header, not the layers, these must be added separtely
      Hwc1LayerList(const hwcval_display_contents_t *sfDisplay);
    };

}

#endif // __HwcvalHwc1Content_h__
