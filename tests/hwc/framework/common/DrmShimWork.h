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

File Name:      DrmShimWork.h

Description:    Drm-related class definitions for Hwc validation Work Queue.

Environment:

Notes:

****************************************************************************/
#ifndef __DrmShimWork_h__
#define __DrmShimWork_h__

// NOTE: HwcTestDefs.h sets defines which are used in the HWC and DRM stack.
// -> to be included before any other HWC or DRM header file.
#include "HwcvalWork.h"

class HwcTestState;
class DrmShimBuffer;
class HwcTestKernel;

namespace Hwcval
{
    namespace Work
    {
        class AddFbItem : public Item
        {
        public:
            uint32_t mBoHandle;
            uint32_t mFbId;
            uint32_t mWidth;
            uint32_t mHeight;
            uint32_t mPixelFormat;

            uint32_t mAuxPitch;
            uint32_t mAuxOffset;
            bool mHasAuxBuffer;
            __u64 mModifier;

        public:
            AddFbItem(int fd, uint32_t boHandle, uint32_t fbId, uint32_t width, uint32_t height, uint32_t pixelFormat);
            AddFbItem(int fd, uint32_t boHandle, uint32_t fbId, uint32_t width, uint32_t height, uint32_t pixelFormat, uint32_t auxPitch, uint32_t auxOffset, __u64 modifier);
            virtual ~AddFbItem();
            virtual void Process();
        };

        class RmFbItem : public Item
        {
        public:
            uint32_t mFbId;

        public:
            RmFbItem(int fd, uint32_t fbId);
            virtual ~RmFbItem();
            virtual void Process();
        };


    } // namespace Work
} // namespace Hwcval

#endif // __DrmShimWork_h__
