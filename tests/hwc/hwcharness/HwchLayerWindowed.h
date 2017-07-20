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

#ifndef __HwchLayerWindowed_h__
#define __HwchLayerWindowed_h__

#include <utils/RefBase.h>
#include <ui/GraphicBuffer.h>

#include "HwchLayer.h"
#include "HwchCoord.h"
#include "HwchDisplay.h"

#define WIDI_WINDOW_OFFSET 100

class HwchLayerWindowed :
    public Hwch::Layer,
    public android::RefBase
{
    private:

    buffer_handle_t mHandle;

    public:

    HwchLayerWindowed(uint32_t width, uint32_t height, buffer_handle_t handle);

    // Getter to return handle
    buffer_handle_t GetHandle(void)
    {
        return mHandle;
    }

    void Send(hwc2_layer_t &hwLayer, hwc_rect_t *visibleRegions,
              uint32_t &visibleRegionCount) override;
    void CalculateRects(Hwch::Display& display) override;
};

#endif // __HwchLayerWindowed_h__
