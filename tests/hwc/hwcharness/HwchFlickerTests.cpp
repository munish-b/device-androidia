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
* File Name:            HwchFlickerTests.cpp
*
* Description:          Implementation of flicker test classes
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#include "HwchFlickerTests.h"
#include "HwchPattern.h"
#include "HwchLayers.h"
#include "HwcTestLog.h"
#include "HwcTestState.h"
#include "HwcTestUtil.h"

REGISTER_TEST(Flicker1)
Hwch::Flicker1Test::Flicker1Test(Hwch::Interface& interface)
  : Hwch::Test(interface)
{
}

// Alternate between full-screen 16-bit and full-screen 32-bit layers.
int Hwch::Flicker1Test::RunScenario()
{
    Hwch::Frame frame(mInterface);

    // 16-bit layer
    Hwch::GameFullScreenLayer game(MaxRel(0), MaxRel(0));

    // 32-bit layer
    Hwch::RGBALayer rgba(MaxRel(0), MaxRel(0));

    for (uint32_t i=0; i<20; ++i)
    {
        frame.Add(game);
        frame.Send(10);
        frame.Remove(game);

        frame.Add(rgba);
        frame.Send(10);
        frame.Remove(rgba);
    }

    return 0;
}

REGISTER_TEST(Flicker2)
Hwch::Flicker2Test::Flicker2Test(Hwch::Interface& interface)
  : Hwch::Test(interface)
{
}

// Generate Max Fifo flicker by:
//   Generate layers which result in 16- and 32-bit planes being used at once
//   Wait for >0.6ms so kernel goes into idle mode
//   Then send just a 32-bit layer to the screen.

int Hwch::Flicker2Test::RunScenario()
{
    Hwch::Frame frame(mInterface);

    // 16-bit layer
    Hwch::GameFullScreenLayer game(MaxRel(0), MaxRel(0));

    // 32-bit layers
    Hwch::NavigationBarLayer nav;
    Hwch::StatusBarLayer status;

    for (uint32_t i=0; i<20; ++i)
    {
        frame.Add(game);
        frame.Add(nav);
        frame.Add(status);
        frame.Send();

        // 0.8ms delay - stimulate idle mode
        usleep(800*1000);

        frame.Remove(game);
        frame.Send(3);
        frame.Remove(nav);
        frame.Remove(status);
    }

    return 0;
}

REGISTER_TEST(Flicker3)
Hwch::Flicker3Test::Flicker3Test(Hwch::Interface& interface)
  : Hwch::Test(interface)
{
}

// Send a 16-bit layer to the screen 3 times
// then wait >0.6ms for kernel to go into idle mode.

int Hwch::Flicker3Test::RunScenario()
{
    Hwch::Frame frame(mInterface);

    Hwch::GameFullScreenLayer game(MaxRel(0), MaxRel(0));
    frame.Add(game);

    for (uint32_t i=0; i<20; ++i)
    {
        frame.Send(3);

        // 0.8ms delay - stimulate idle mode
        usleep(800*1000);
    }

    return 0;
}


