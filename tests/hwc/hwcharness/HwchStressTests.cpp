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
* File Name:            HwchStressTests.cpp
*
* Description:          Implementation of stress test classes
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#include "HwchStressTests.h"
#include "HwchLayers.h"
#include "HwcTestLog.h"
#include "HwcTestState.h"
#include "HwcTestUtil.h"

#include <utils/Thread.h>

Hwch::BufferAllocator::BufferAllocator()
{
    run("Hwch::BufferAllocator", android::PRIORITY_NORMAL);
}

Hwch::BufferAllocator::~BufferAllocator()
{
}

android::status_t Hwch::BufferAllocator::readyToRun()
{
    return android::NO_ERROR;
}

bool Hwch::BufferAllocator::threadLoop()
{
    uint32_t ctr = 0;
    Hwch::System& system = Hwch::System::getInstance();
    Hwch::BufferDestroyer & bd = system.GetBufferDestroyer();

    while (!exitPending())
    {
        {
            // Create buffers continually on the thread
            android::sp<android::GraphicBuffer> buf = new android::GraphicBuffer(32, 32, HAL_PIXEL_FORMAT_RGBA_8888,
                GRALLOC_USAGE_HW_FB | GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_RENDER);

            // ALlow 50% to go out of scope immediately.
            // The rest go on to the buffer destroyer thread, until it is 50% full.
            if ((++ctr & 1) == 0)
            {
                if (bd.Size() < (bd.MaxSize() / 2))
                {
                    bd.Push(buf);
                }
            }
        }

        sleep(0);
    }

    HWCLOGI("Background thread created and destroyed %d buffers.", ctr);

    return false;
}

#define NUM_ALLOCATORS 10

REGISTER_TEST(BufferStress)
Hwch::BufferStressTest::BufferStressTest(Hwch::Interface& interface)
  : Hwch::OptionalTest(interface)
{
}

int Hwch::BufferStressTest::RunScenario()
{
    android::Vector<android::sp<Hwch::BufferAllocator> > ba;

    for (uint32_t i=0; i<NUM_ALLOCATORS; ++i)
    {
        ba.push_back( new Hwch::BufferAllocator);
    }

    Hwch::Frame frame(mInterface);

    int32_t screenWidth = mSystem.GetDisplay(0).GetWidth();
    //int32_t screenHeight = mSystem.GetDisplay(0).GetHeight();

    Hwch::WallpaperLayer layer1;
    frame.Add(layer1);

    uint32_t testIterations = GetIntParam("test_iterations", 10);

    for (uint32_t i=0; i<testIterations; ++i)
    {
        for (int j=100; j<screenWidth; j+= 32)
        {
            Hwch::NV12VideoLayer video(200, j);
            video.SetLogicalDisplayFrame( LogDisplayRect(50,200,j,500));
            frame.Add(video);
            frame.Send();
        }
    }

    for (uint32_t i=0; i<ba.size(); ++i)
    {
        ba[i]->requestExitAndWait();
    }

    return 0;
}


