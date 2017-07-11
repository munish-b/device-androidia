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
 * @file    HwchLayerWindowed.cpp
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    27th February 2015
 * @brief   Layer sub-class for creating on-screen windows.
 *
 * @details This file sub-classes HwchLayer to allow the Widi and Virtual Display
 * mechanisms to display their output buffers in a window on the screen.
 *
 *****************************************************************************/

#include "HwcTestState.h"
#include "HwchSystem.h"
#include "HwchLayerWindowed.h"

HwchLayerWindowed::HwchLayerWindowed(uint32_t width, uint32_t height, buffer_handle_t handle):
    Hwch::Layer("Windowed", width, height, HAL_PIXEL_FORMAT_RGBA_8888, 0),
    mHandle(handle)
{
    Hwch::System& system = Hwch::System::getInstance();
    uint32_t panel_width = system.GetDisplay(0).GetWidth();
    uint32_t panel_height = system.GetDisplay(0).GetHeight();

    SetIgnoreScreenRotation(true);

    // Trigger a warning if the width and height of the Widi window exceeds the size of the panel.
    if (((WIDI_WINDOW_OFFSET + width) > panel_width) ||
         (WIDI_WINDOW_OFFSET + height) > panel_height)
    {
        HWCLOGW("The Widi window parameters are too large for the panel - capping to screen edges.\n");
    }

    SetLogicalDisplayFrame(Hwch::LogDisplayRect(WIDI_WINDOW_OFFSET, WIDI_WINDOW_OFFSET,
        ((WIDI_WINDOW_OFFSET + width) <= panel_width) ? WIDI_WINDOW_OFFSET+width : panel_width,
        ((WIDI_WINDOW_OFFSET + height) <= panel_height) ? WIDI_WINDOW_OFFSET+height : panel_height));

    SetCrop(Hwch::LogCropRect(0, 0, panel_width, panel_height));
};

void HwchLayerWindowed::Send(hwc_layer_1_t& hwLayer, hwc_rect_t* visibleRegions, uint32_t& visibleRegionCount)
{
    // Setup the layer properties and the visible regions
    AssignLayerProperties(hwLayer, mHandle);
    AssignVisibleRegions(hwLayer, visibleRegions, visibleRegionCount);
};

void HwchLayerWindowed::CalculateRects(Hwch::Display& display)
{
    // Just calculate the source crop and the display frame
    CalculateSourceCrop(display);
    CalculateDisplayFrame(display);

    HWCLOGI("CalculateRects(%s): LogCrop %s Crop %s", mName.string(), mLogicalCropf.Str("%f"),mSourceCropf.Str());
    HWCLOGI("CalculateRects(%s): LogDF   %s DF   %s", mName.string(), mLogicalDisplayFrame.Str("%d"),mDisplayFrame.Str());
}
