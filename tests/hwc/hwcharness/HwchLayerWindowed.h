/****************************************************************************
 *
 * Copyright (c) Intel Corporation (2014).
 *
 * DISCLAIMER OF WARRANTY
 * NEITHER INTEL NOR ITS SUPPLIERS MAKE ANY REPRESENTATION OR WARRANTY OR
 * CONDITION OF ANY KIND WHETHER EXPRESS OR IMPLIED (EITHER IN FACT OR BY
 * OPERATION OF LAW) WITH RESPECT TO THE SOURCE CODE.  INTEL AND ITS SUPPLIERS
 * EXPRESSLY DISCLAIM ALL WARRANTIES OR CONDITIONS OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  INTEL AND ITS SUPPLIERS DO NOT WARRANT
 * THAT THE SOURCE CODE IS ERROR-FREE OR THAT OPERATION OF THE SOURCE CODE WILL
 * BE SECURE OR UNINTERRUPTED AND HEREBY DISCLAIM ANY AND ALL LIABILITY ON
 * ACCOUNT THEREOF.  THERE IS ALSO NO IMPLIED WARRANTY OF NON-INFRINGEMENT.
 * SOURCE CODE IS LICENSED TO LICENSEE ON AN "AS IS" BASIS AND NEITHER INTEL
 * NOR ITS SUPPLIERS WILL PROVIDE ANY SUPPORT, ASSISTANCE, INSTALLATION,
 * TRAINING OR OTHER SERVICES.  INTEL AND ITS SUPPLIERS WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.
 *
 * @file    HwchLayerWindowed.h
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    17th February 2015
 * @brief   Layer sub-class for creating on-screen windows.
 *
 * @details This file sub-classes HwchLayer to allow the Widi and Virtual Display
 * mechanisms to display their output buffers in a window on the screen.
 *
 *****************************************************************************/

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

    void Send(hwc_layer_1_t& hwLayer, hwc_rect_t* visibleRegions, uint32_t& visibleRegionCount) override;
    void CalculateRects(Hwch::Display& display) override;
};

#endif // __HwchLayerWindowed_h__
