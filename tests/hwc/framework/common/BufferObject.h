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

File Name:      BufferObject.h

Description:    Class definition for buffer object tracking classes.
                Keeps track of usage of a gralloc buffer within HWC.

Environment:

Notes:

****************************************************************************/
#ifndef __BufferObject_h__
#define __BufferObject_h__

#include <stdint.h>
#include <utils/KeyedVector.h>
#include <utils/RefBase.h>

#include "HwcTestDefs.h"

class DrmShimPlane;
class DrmShimBuffer;

class HwcTestBufferObject : public android::RefBase
{
public:
    android::wp<DrmShimBuffer> mBuf;
    int mFd;
    uint32_t mBoHandle;

public:
    HwcTestBufferObject(int fd, uint32_t boHandle);
    HwcTestBufferObject(const HwcTestBufferObject& rhs);
    virtual ~HwcTestBufferObject();

    char* IdStr(char* str, uint32_t len=HWCVAL_DEFAULT_STRLEN-1);
    int FullIdStr(char* str, uint32_t len=HWCVAL_DEFAULT_STRLEN-1);
    virtual HwcTestBufferObject* Dup();
};

#endif // __BufferObject_h__
