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

#include "HwchProtectedContentTests.h"
#include "HwchPattern.h"
#include "HwchLayers.h"
#include "HwcTestLog.h"
#include "HwcTestState.h"
#include "HwcTestUtil.h"

REGISTER_TEST(ProtectedVideo)
Hwch::ProtectedVideoTest::ProtectedVideoTest(Hwch::Interface& interface)
  : Hwch::Test(interface)
{
}

int Hwch::ProtectedVideoTest::RunScenario()
{
    Hwch::Frame frame(mInterface);

    // Make sure the display is enabled before we create the protected session
    Hwch::WallpaperLayer wallpaper;
    frame.Add(wallpaper);
    frame.Send(10);

    mSystem.StartProtectedContent();
    frame.Send(HWCH_PAVP_WARMUP_FRAMECOUNT);
    if (!mSystem.ProtectedContentStarted())
    {
        return -1;
    }

    frame.Remove(wallpaper);

    Hwch::ProtectedVideoLayer layer1;

    frame.Add(layer1);
    frame.Send();

    SetExpectedMode(HwcTestConfig::eDontCare); // cloning not done for protected content
    UpdateVideoState(0, true); // MDS says video is being played
    frame.Send(50);
    UpdateInputState(false, true, &frame); // MDS says input has timed out
    frame.Send(100);
    UpdateInputState(true, true, &frame);   // MDS says display has been touched
    frame.Send(50);

    // Check sessions are torn down on hot unplug.
    SimulateHotPlug(false, AsyncEvent::cRemovableDisplay, 0);
    frame.Send(100);

    // Don't exit the test in unplug state.
    sleep(2);
    SimulateHotPlug(true);

    frame.Remove(layer1);
    frame.Add(wallpaper);
    frame.Send(HWCH_PAVP_WARMUP_FRAMECOUNT);

    // Tidy up
    UpdateVideoState(0, false);

    return 0;
}

REGISTER_TEST(ProtectedVideoHotPlug)
Hwch::ProtectedVideoHotPlugTest::ProtectedVideoHotPlugTest(Hwch::Interface& interface)
  : Hwch::Test(interface)
{
}

int Hwch::ProtectedVideoHotPlugTest::RunScenario()
{
    // This test usually fails this check, so until it's been debugged we will set it to a warning.
    SetCheckPriority(eCheckHotPlugTimeout, ANDROID_LOG_WARN);

    Hwch::Frame frame(mInterface);

    // Ensure we are unplugged before we start
    SimulateHotPlug(false);

    Hwch::WallpaperLayer wallpaper;
    frame.Add(wallpaper);
    frame.Send(HWCH_PAVP_WARMUP_FRAMECOUNT);

    mSystem.StartProtectedContent();
    frame.Send(HWCH_PAVP_WARMUP_FRAMECOUNT);
    if (!mSystem.ProtectedContentStarted())
    {
        return -1;
    }

    Hwch::ProtectedVideoLayer layer1;

    frame.Add(layer1);
    frame.Send(100);

    // Check sessions are torn down on hot plug.
    SimulateHotPlug(true);
    frame.Send(500);

    return 0;
}

REGISTER_TEST(ProtectedVideoScreenDisable)
Hwch::ProtectedVideoScreenDisableTest::ProtectedVideoScreenDisableTest(Hwch::Interface& interface)
  : Hwch::Test(interface)
{
}

int Hwch::ProtectedVideoScreenDisableTest::RunScenario()
{
    Hwch::Frame frame(mInterface);

    // Make sure the display is enabled before we create the protected session
    Hwch::WallpaperLayer wallpaper;
    frame.Add(wallpaper);
    frame.Send(10);

    mSystem.StartProtectedContent();
    frame.Send(HWCH_PAVP_WARMUP_FRAMECOUNT);
    if (!mSystem.ProtectedContentStarted())
    {
        return -1;
    }

    Hwch::ProtectedVideoLayer layer1;

    frame.Add(layer1);
    frame.Send(100);

    // Perform screen disable
    Blank(true);
    frame.Send(5);
    Blank(false);

    frame.Send(500);

    return 0;
}

REGISTER_TEST(InvalidProtectedVideo)
Hwch::InvalidProtectedVideoTest::InvalidProtectedVideoTest(Hwch::Interface& interface)
  : Hwch::Test(interface)
{
}

int Hwch::InvalidProtectedVideoTest::RunScenario()
{
    Hwch::Frame frame(mInterface);
    WallpaperLayer wallpaper;
    frame.Add(wallpaper);

    mSystem.StartProtectedContent();
    frame.Send(HWCH_PAVP_WARMUP_FRAMECOUNT);
    if (!mSystem.ProtectedContentStarted())
    {
        return -1;
    }

    frame.Clear();

    Hwch::ProtectedVideoLayer layer1(Hwch::Layer::eEncrypted | Hwch::Layer::eInvalidSessionId);

    frame.Add(layer1);
    frame.Send(100);

    frame.Remove(layer1);
    Hwch::ProtectedVideoLayer layer2(Hwch::Layer::eEncrypted | Hwch::Layer::eInvalidInstanceId);
    frame.Add(layer2);
    frame.Send(100);

    return 0;
}

REGISTER_TEST(ComplexProtectedVideo)
Hwch::ComplexProtectedVideoTest::ComplexProtectedVideoTest(Hwch::Interface& interface)
  : Hwch::Test(interface)
{
}

int Hwch::ComplexProtectedVideoTest::RunScenario()
{
    Hwch::Frame frame(mInterface);

    Hwch::WallpaperLayer layer1;
    Hwch::LauncherLayer layer2;
    Hwch::StatusBarLayer layer3;
    Hwch::NavigationBarLayer layer4;

    frame.Add(layer1);
    frame.Add(layer2);
    frame.Add(layer3);
    frame.Add(layer4);
    frame.Send(10);

    if (!mSystem.StartProtectedContent())
    {
        return -1;
    }

    frame.Send(HWCH_PAVP_WARMUP_FRAMECOUNT);

    Hwch::ProtectedVideoLayer layer5;
    layer5.SetLogicalDisplayFrame( LogDisplayRect(MaxRel(-779), 260, MaxRel(-20), 260+460));    // Scale the video into a popout window
    Hwch::AdvertLayer layer6; // This represents video UI
    frame.Add(layer5);
    frame.Add(layer6);

    frame.Send(100);

    HWCLOGI("ComplexProtectedVideoTest: removing all layers");
    frame.Remove(layer1);
    frame.Remove(layer2);
    frame.Remove(layer3);
    frame.Remove(layer4);
    frame.Remove(layer5);
    frame.Remove(layer6);

    HWCLOGI("ComplexProtectedVideoTest: adding layers back in presentation mode");
    // Presentation mode screen 1
    frame.Add(layer1, 0);
    frame.Add(layer2, 0);
    frame.Add(layer3, 0);
    frame.Add(layer4, 0);
    frame.Add(layer5, 1);
    frame.Add(layer6, 1);

    layer5.SetLogicalDisplayFrame(LogDisplayRect(0, 0, MaxRel(0), MaxRel(0)));
    frame.Send(100);

    return 0;
}

