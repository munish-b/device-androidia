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
* File Name:            HwchBufferDestroyer.cpp
*
* Description:          Buffer destroyer thread for testing asynchronous buffer destruction.
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#include "HwchBufferDestroyer.h"
#include "HwchSystem.h"
#include "HwcTestState.h"

Hwch::BufferDestroyer::BufferDestroyer()
  : EventThread("BufferDestroyer")
{
    HWCLOGD("Starting BufferDestroyer thread");
    EnsureRunning();
}

Hwch::BufferDestroyer::~BufferDestroyer()
{
}

bool Hwch::BufferDestroyer::threadLoop()
{
    // Just pull each buffer from the event queue and allow it to be destroyed.

    while (true)
    {
        buffer_handle_t handle = 0;
        HWCLOGD("Size=%d", Size());

        HWCLOGD("Waiting for onSet and 10 buffers in queue before destroying buffers...");
        while (Size() < 10)
        {
            HwcTestState::getInstance()->WaitOnSetCondition();
        }

        HWCLOGD("Start destroying buffers, now %d in queue", Size());

        while (Size() > 0)
        {
            android::sp<android::GraphicBuffer> buf;
            if (ReadWait(buf))
            {
                handle = buf->handle;
                HWCLOGD("Destroying buffer handle %p", handle);
            }
        }
    }

    return true;
}
