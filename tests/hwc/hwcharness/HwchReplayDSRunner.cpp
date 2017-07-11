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
 * @file    Hwch::ReplayDSRunner.cpp
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    12th June 2014
 * @brief   Implementation of the dumpsys runner.
 *
 * @details This file implements the dumpsys runner i.e. the piece of code
 * that coordinates the replaying of frames through the HWC (for dumpsys files).
 *
 *****************************************************************************/

#include "HwchFrame.h"
#include "HwchLayer.h"
#include "HwchSystem.h"
#include "HwcTestLog.h"

#include "HwchReplayDSRunner.h"
#include "HwchReplayDSLayers.h"

bool Hwch::ReplayDSRunner::AddLayers(Hwch::Frame& frame, int32_t display,
                                   layer_cache_t& layer_cache)
{
    bool retVal = true;

    std::string line;
    while (std::getline(mFile, line))
    {
        if (line.empty() || !mParser->IsDSLayer(line))
        {
            continue;
        }
        else
        {
            // We have seen a valid layer - increment the layer count
            mStats.parsed_layer_count++;
        }

        if (mParser->IsDSLayerFramebufferTarget(line))
        {
            // The framebuffer target is the last in the list
            // and should not be added to the frame
            break;
        }
        else
        {
            // We have seen a valid layer that is not a framebuffer target
            mStats.processed_layer_count++;
        }

        // The line matches the pattern for a dumpsys layer (and is not marked
        // 'FB TARGET'). Process the layer based on its profile (if specified)
        std::string profile;
        mParser->ParseDSProfile(line, profile);

        Hwch::ReplayLayer *layer =
            profile == "VideoPlayer" ? new Hwch::ReplayDSLayerVideoPlayer() :
            profile == "Application" ? new Hwch::ReplayDSLayerApplication() :
            profile == "Transparent" ? new Hwch::ReplayDSLayerTransparent() :
            profile == "SemiTransparent" ? new Hwch::ReplayDSLayerSemiTransparent() :
            profile == "DialogBox" ? new Hwch::ReplayDSLayerDialogBox() :
            profile == "StatusBar" ? new Hwch::ReplayDSLayerStatusBar() :
            profile == "NavigationBar" ? new Hwch::ReplayDSLayerNavigationBar() :
            static_cast<Hwch::ReplayLayer*>(NULL);

        // Default settings - ParseDSLayer will overwrite these (if specified)
        if (!layer)
        {
            layer = new ReplayLayer("Replay", 0, 0, HAL_PIXEL_FORMAT_RGBA_8888, 3);
            layer->SetPattern(mSystem.GetPatternMgr().CreateHorizontalLinePtn(
                layer->GetFormat(), 0.0, rand(), rand()));
        }

        // Cache the pointer for automatic deallocation
        layer_cache.add(layer);

        // Parse the layer and add it to the current frame
        if (mParser->ParseDSLayer(line, *layer))
        {
            frame.Add(*layer, display);
        }
    }

    return retVal;
}

void Hwch::ReplayDSRunner::PrintStatistics(void)
{
    // Print the replay statistics
    std::printf("Dumpsys snapshot replay complete. Statistics are as follows:\n"
        "\t%d frames parsed (for all displays)\n"
        "\t%d layers parsed (including framebuffer targets)\n"
        "\t%d frames sent to the HWC\n"
        "\t%d layers sent to HWC\n",
        mStats.parsed_frame_count,
        mStats.parsed_layer_count,
        mStats.hwc_frame_count,
        mStats.processed_layer_count);
}

int Hwch::ReplayDSRunner::RunScenario()
{
    Hwch::Frame frame(mInterface);
    bool success = true;

    // Create a cache of the layer pointers for deallocation
    layer_cache_t layer_cache;

    // Scenario data
    int32_t display = 0, width = 0, height = 0;

    // Parse the replay file line-by-line
    std::string line;
    while (std::getline(mFile, line))
    {
        if (!line.empty() && mParser->ParseDSDisplay(line, display, width, height))
        {
            HWCLOGI("Parsed display: %d width: %d height: %d", display, width, height);
            mStats.parsed_frame_count++;

            // Look for and add the layers to the frame
            success = AddLayers(frame, display, layer_cache);
        }
    }

    /* The HWC harness currently only supports frames with at least one layer on
     * display 0 (i.e. the panel on a Baytrail FFRD8) */
    if (frame.NumLayers(0))
    {
        // Rotate the frame if the scenario was performed on a portrait panel
        int32_t screenWidth = Hwch::System::getInstance().GetDisplay(0).GetWidth();
        int32_t screenHeight = Hwch::System::getInstance().GetDisplay(0).GetHeight();
        if ((screenWidth == height) && (screenHeight == width))
        {
            HWCLOGI("Rotating frame - panel dimensions are %dx%d, whereas scenario "
                    "dimensions are %dx%d", screenWidth, screenHeight, width, height);
            frame.RotateBy(Hwch::eRotate270);
        }
        frame.Send(mNumFrames);
        mStats.hwc_frame_count += mNumFrames;
    }

    PrintStatistics();

    return success ? 0 : -1;
}
