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
* File Name:            HwchModeTests.cpp
*
* Description:          Implementation of disaply mode test classes
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#include "HwchModeTests.h"
#include "HwchPattern.h"
#include "HwchLayers.h"
#include "HwchPngImage.h"
#include "HwcTestLog.h"
#include "HwcTestState.h"
#include "HwcTestUtil.h"

// REGISTER_TEST(VideoModes)
Hwch::VideoModesTest::VideoModesTest(Hwch::Interface& interface)
  : Hwch::Test(interface)
{
}

int Hwch::VideoModesTest::RunScenario()
{
    Hwch::Frame frame(mInterface);

    bool doModeSet = true;
    Hwch::Display* display = &mSystem.GetDisplay(1);

    if (!display->IsConnected())
    {
        doModeSet = false;
        display = &mSystem.GetDisplay(0);
    }

    uint32_t modeCount = display->GetModes();


    Hwch::NV12VideoLayer video;
    Hwch::WallpaperLayer wallpaper;

    // Set the video update frequency
    video.GetPattern().SetUpdateFreq(50);

    // Make sure HWC is fully started before we set mode
    frame.Add(wallpaper);
    frame.Send(10);

#define HWCVAL_RESET_PREFERRED_MODE_NOT_WORKING
#ifdef HWCVAL_RESET_PREFERRED_MODE_NOT_WORKING
    uint32_t entryMode;
    int st = display->GetCurrentMode(entryMode);
    ALOG_ASSERT(st);
#endif

    for (uint32_t m = 0; m<modeCount; ++m)
    {
        if (doModeSet)
        {
            HWCLOGD("Setting display mode %d/%d", m, modeCount);
            display->SetMode(m);
        }

        SetExpectedMode(HwcTestConfig::eOn);
        frame.Send(20);

        frame.Remove(wallpaper);
        UpdateVideoState(0, true, 50); // MDS says video is being played
        frame.Add(video);

        frame.Send(30);
        UpdateInputState(false, true, &frame); // MDS says input has timed out
        frame.Send(30);

        UpdateInputState(true, true, &frame);   // MDS says display has been touched
        frame.Send(20);

        frame.Remove(video);
        UpdateVideoState(0, false);
        frame.Add(wallpaper);

        frame.Send(20);
    }

    if (doModeSet)
    {
#ifdef HWCVAL_RESET_PREFERRED_MODE_NOT_WORKING
        HWCLOGD("Restoring entry mode");
        display->SetMode(entryMode);
#else
        HWCLOGD("Clearing display mode");
        display->ClearMode();
#endif
    }

    SetExpectedMode(HwcTestConfig::eDontCare);
    frame.Send(30);

    return 0;
}
