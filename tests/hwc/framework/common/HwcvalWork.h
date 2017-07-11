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

File Name:      HwcvalWork.h

Description:    Class definitions for Hwc validation Work Queue.

Environment:

Notes:

****************************************************************************/
#ifndef __HwcvalWork_h__
#define __HwcvalWork_h__

// NOTE: HwcTestDefs.h sets defines which are used in the HWC and DRM stack.
// -> to be included before any other HWC or DRM header file.
#include "HwcTestDefs.h"
#include "EventQueue.h"
#include "utils/RefBase.h"
#include "utils/StrongPointer.h"

class HwcTestState;
class DrmShimBuffer;
class HwcTestKernel;

namespace Hwcval
{
    namespace Work
    {

        class Item : public android::RefBase
        {
        public:
            int mFd;

        public:
            Item (int fd);
            virtual ~Item();
            virtual void Process() = 0;
        };

        class GemOpenItem : public Item
        {
        public:
            int mId;
            uint32_t mBoHandle;

        public:
            GemOpenItem(int fd, int id, uint32_t boHandle);
            virtual ~GemOpenItem();
            virtual void Process();
        };

        class GemCloseItem : public Item
        {
        public:
            uint32_t mBoHandle;

        public:
            GemCloseItem(int fd, uint32_t boHandle);
            virtual ~GemCloseItem();
            virtual void Process();
        };

        class GemFlinkItem : public Item
        {
        public:
            int mId;
            uint32_t mBoHandle;

        public:
            GemFlinkItem(int fd, int id, uint32_t boHandle);
            virtual ~GemFlinkItem();
            virtual void Process();
        };

        class GemCreateItem : public Item
        {
        public:
             uint32_t mBoHandle;

        public:
            GemCreateItem(int fd, uint32_t boHandle);
            virtual ~GemCreateItem();
            virtual void Process();
        };

        class GemWaitItem : public Item
        {
        public:
            uint32_t mBoHandle;
            int32_t mStatus;
            int64_t mDelayNs;

        public:
            GemWaitItem(int fd, uint32_t boHandle, int32_t status, int64_t delayNs);
            virtual ~GemWaitItem();
            virtual void Process();
        };

        class PrimeItem : public Item
        {
        public:
            uint32_t mBoHandle;
            int mDmaHandle;

        public:
            PrimeItem(int fd, uint32_t boHandle, int dmaHandle);
            virtual ~PrimeItem();
            virtual void Process();
        };

        class BufferFreeItem : public Item
        {
        public:
            buffer_handle_t mHandle;

        public:
            BufferFreeItem(buffer_handle_t handle);
            virtual ~BufferFreeItem();
            virtual void Process();
        };

        class Queue : public EventQueue<android::sp<Item>, HWCVAL_MAX_GEM_EVENTS>
        {
        public:

            //-----------------------------------------------------------------------------
            // Constructor & Destructor
            Queue();
            virtual ~Queue();

            void Process();
        };

    } // namespace Work
} // namespace Hwcval



#endif // __HwcvalWork_h__
