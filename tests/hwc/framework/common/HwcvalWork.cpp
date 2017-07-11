/****************************************************************************

Copyright (c) Intel Corporation (2014).

DISCLAIMER OF WARRANTY
NEITHER INTEL NOR ITS SUPPLIERS MAKE ANY REPRESENTATION OR WARRANTY OR
CONDITION OF ANY KIND WHETHER EXPRESS OR IMPLIED (EITHER IN FACT OR BY
OPERATION OF LAW) WITH RESPECT TO THE SOURCE CODE.  INTEL AND ITS SUPPLIERS
EXPRESSLY DISCLAIM ALL WARRANTIES OR CONDITIONS OF MERCHANTABILITY OR
FITNESS FOR A PARTICULAR PURPOSE.  INTEL AND ITS SUPPLIERS DO NOT WARRANT
THAT THE SOURCE CODE IS ERROR-FREE OR THAT OPERATION OF THE SOURCE CODE WILL
BE SECURE OR UNINTERRUPTED AND HEREBY DISCLAIM ANY AND ALL LIABILITY ON
ACCOUNT THEREOF.  THERE IS ALSO NO IMPLIED WARRANTY OF NON-INFRINGEMENT.
SOURCE CODE IS LICENSED TO LICENSEE ON AN "AS IS" BASIS AND NEITHER INTEL
NOR ITS SUPPLIERS WILL PROVIDE ANY SUPPORT, ASSISTANCE, INSTALLATION,
TRAINING OR OTHER SERVICES.  INTEL AND ITS SUPPLIERS WILL NOT PROVIDE ANY
UPDATES, ENHANCEMENTS OR EXTENSIONS.

File Name:      HwcvalWork.cpp

Description:    Class implementations for Hwc validation Work Queue.

Environment:

Notes:

****************************************************************************/
#include "HwcvalWork.h"
#include "HwcTestKernel.h"
#include "HwcTestState.h"

#ifdef HWCVAL_RESOURCE_LEAK_CHECKING
static uint32_t itemCount = 0;
#endif

Hwcval::Work::Item::Item(int fd)
  : mFd(fd)
{
#ifdef HWCVAL_RESOURCE_LEAK_CHECKING
    if (++itemCount > 500)
    {
        HWCLOGW("%d work items in transit", itemCount);
    }
#endif
}

Hwcval::Work::Item::~Item()
{
#ifdef HWCVAL_RESOURCE_LEAK_CHECKING
    --itemCount;
#endif
}

// GemOpenItem
Hwcval::Work::GemOpenItem::GemOpenItem(int fd, int id, uint32_t boHandle)
  : Item(fd),
    mId(id),
    mBoHandle(boHandle)
{
}

Hwcval::Work::GemOpenItem:: ~GemOpenItem()
{
}

void Hwcval::Work::GemOpenItem::Process()
{
    HwcTestState::getInstance()->GetTestKernel()->DoGem(*this);
}

// GemCloseItem
Hwcval::Work::GemCloseItem::GemCloseItem(int fd, uint32_t boHandle)
  : Item(fd),
    mBoHandle(boHandle)
{
}

Hwcval::Work::GemCloseItem:: ~GemCloseItem()
{
}

void Hwcval::Work::GemCloseItem::Process()
{
    HwcTestState::getInstance()->GetTestKernel()->DoGem(*this);
}

// GemCreateItem
Hwcval::Work::GemCreateItem::GemCreateItem(int fd, uint32_t boHandle)
  : Item(fd),
    mBoHandle(boHandle)
{
}

Hwcval::Work::GemCreateItem:: ~GemCreateItem()
{
}

void Hwcval::Work::GemCreateItem::Process()
{
    HwcTestState::getInstance()->GetTestKernel()->DoGem(*this);
}

// GemFlinkItem
Hwcval::Work::GemFlinkItem::GemFlinkItem(int fd, int id, uint32_t boHandle)
  : Item(fd),
    mId(id),
    mBoHandle(boHandle)
{
}

Hwcval::Work::GemFlinkItem:: ~GemFlinkItem()
{
}

void Hwcval::Work::GemFlinkItem::Process()
{
    HwcTestState::getInstance()->GetTestKernel()->DoGem(*this);
}

Hwcval::Work::GemWaitItem::GemWaitItem(int fd, uint32_t boHandle, int32_t status, int64_t delayNs)
  : Item(fd),
    mBoHandle(boHandle),
    mStatus(status),
    mDelayNs(delayNs)
{
}

Hwcval::Work::GemWaitItem::~GemWaitItem()
{
}

void Hwcval::Work::GemWaitItem::Process()
{
    HwcTestState::getInstance()->GetTestKernel()->DoGem(*this);
}


// PrimeItem
Hwcval::Work::PrimeItem::PrimeItem(int fd, uint32_t boHandle, int dmaHandle)
  : Item(fd),
    mBoHandle(boHandle),
    mDmaHandle(dmaHandle)
{
}

Hwcval::Work::PrimeItem:: ~PrimeItem()
{
}

void Hwcval::Work::PrimeItem::Process()
{
    HwcTestState::getInstance()->GetTestKernel()->DoPrime(*this);
}

// Queue
Hwcval::Work::Queue::Queue()
  : EventQueue("Hwcval::Work::Queue")
{
}

Hwcval::Work::BufferFreeItem::BufferFreeItem(buffer_handle_t handle)
  : Item(0),
    mHandle(handle)
{
}

Hwcval::Work::BufferFreeItem::~BufferFreeItem()
{
}

void Hwcval::Work::BufferFreeItem::Process()
{
    HwcTestState::getInstance()->GetTestKernel()->DoBufferFree(*this);
}

Hwcval::Work::Queue::~Queue()
{
}

void Hwcval::Work::Queue::Process()
{
    ATRACE_CALL();
    android::sp<Item> item;

    while (Pop(item))
    {
        item->Process();
    }
};
