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

File Name:      DrmShimWork.cpp

Description:    Drm-related class implementation for Hwc validation Work Queue.

Environment:

Notes:

****************************************************************************/
#include "DrmShimWork.h"
#include "DrmShimChecks.h"

// AddFbItem
Hwcval::Work::AddFbItem::AddFbItem(int fd, uint32_t boHandle, uint32_t fbId, uint32_t width, uint32_t height, uint32_t pixelFormat)
  : Item(fd),
    mBoHandle(boHandle),
    mFbId(fbId),
    mWidth(width),
    mHeight(height),
    mPixelFormat(pixelFormat),
    mAuxPitch(0),
    mAuxOffset(0)
{
    mHasAuxBuffer = false;
}

Hwcval::Work::AddFbItem::AddFbItem(int fd, uint32_t boHandle, uint32_t fbId, uint32_t width, uint32_t height, uint32_t pixelFormat, uint32_t auxPitch, uint32_t auxOffset, __u64 modifier)
  : Item(fd),
    mBoHandle(boHandle),
    mFbId(fbId),
    mWidth(width),
    mHeight(height),
    mPixelFormat(pixelFormat),
    mAuxPitch(auxPitch),
    mAuxOffset(auxOffset),
    mModifier(modifier)
{
    mHasAuxBuffer = true;
}

Hwcval::Work::AddFbItem::~AddFbItem()
{
}

void Hwcval::Work::AddFbItem::Process()
{
    DrmShimChecks* checks = static_cast<DrmShimChecks*>(HwcTestState::getInstance()->GetTestKernel());
    checks->DoWork(*this);
}

// RmFbItem
Hwcval::Work::RmFbItem::RmFbItem(int fd, uint32_t fbId)
  : Item(fd),
    mFbId(fbId)
{
}

Hwcval::Work::RmFbItem::~RmFbItem()
{
}

void Hwcval::Work::RmFbItem::Process()
{
    DrmShimChecks* checks = static_cast<DrmShimChecks*>(HwcTestState::getInstance()->GetTestKernel());
    checks->DoWork(*this);
}
