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

File Name:      BufferObject.cpp

Description:    Class implementation for HwcTestBufferObject class.
                Keeps track of usage of a gralloc buffer object within HWC.

Environment:

Notes:

****************************************************************************/

#include "DrmShimBuffer.h"
#include "BufferObject.h"
#include "HwcTestState.h"

// HwcTestBufferObject

#ifdef HWCVAL_RESOURCE_LEAK_CHECKING
static uint32_t bufferObjectCount = 0;
#endif

static void Count()
{
#ifdef HWCVAL_RESOURCE_LEAK_CHECKING
    if (++bufferObjectCount > 500)
    {
        HWCLOGW("%d buffer objects created.", bufferObjectCount);
    }
#endif
}

HwcTestBufferObject::HwcTestBufferObject(int fd, uint32_t boHandle)
  : mFd(fd),
    mBoHandle(boHandle)
{
    Count();
    HWCLOGD_COND(eLogBuffer, "HwcTestBufferObject::HwcTestBufferObject() Created bo@%p", this);
}

HwcTestBufferObject::HwcTestBufferObject(const HwcTestBufferObject& rhs)
  : android::RefBase(),
    mBuf(rhs.mBuf),
    mFd(rhs.mFd),
    mBoHandle(rhs.mBoHandle)
{
    Count();
    HWCLOGD_COND(eLogBuffer, "HwcTestBufferObject::HwcTestBufferObject(&rhs) Created bo@%p", this);
}

HwcTestBufferObject::~HwcTestBufferObject()
{
#ifdef HWCVAL_RESOURCE_LEAK_CHECKING
    --bufferObjectCount;
#endif
    HWCLOGD_COND(eLogBuffer, "HwcTestBufferObject::~HwcTestBufferObject Deleted bo@%p", this);
}

char* HwcTestBufferObject::IdStr(char* str, uint32_t len)
{
    FullIdStr(str, len);
    return str;
}

int HwcTestBufferObject::FullIdStr(char* str, uint32_t len)
{
    uint32_t n = snprintf(str, len, "bo@%p fd %d boHandle 0x%x", this, mFd, mBoHandle);

    return n;
}

HwcTestBufferObject* HwcTestBufferObject::Dup()
{
    return new HwcTestBufferObject(*this);
}

