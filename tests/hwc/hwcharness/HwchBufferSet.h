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
* File Name:            HwchBufferSet.h
*
* Description:          Buffer Set class definition
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#ifndef __HwchBufferSet_h__
#define __HwchBufferSet_h__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>


#include <utils/Vector.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include "GrallocClient.h"
#include <ui/GraphicBuffer.h>
#include <hardware/hwcomposer.h>

#include "HwchPattern.h"
#include "HwchDefs.h"


namespace Hwch
{
    class BufferSet : public android::RefBase
    {
        private:

            struct FencedBuffer
            {
                android::sp<android::GraphicBuffer> mBuf;
                int mReleaseFenceFd;
                uint32_t mParam;

                FencedBuffer(android::GraphicBuffer* buf=0, int fenceFd=-1)
                  : mBuf(buf),
                    mReleaseFenceFd(fenceFd),
                    mParam(HWCH_BUFFERPARAM_UNDEFINED)
                {
                }
            };

            uint32_t mNumBuffers;
            uint32_t mCurrentBuffer;
            uint32_t mNextBuffer;
            uint32_t mWidth;
            uint32_t mHeight;
            uint32_t mFormat;
            uint32_t mUsage;
            uint64_t mLastTimestamp;

            // Protection State
            bool mEncrypted;
#ifdef HWCVAL_BUILD_PAVP
            uint32_t mSessionId;
            uint32_t mInstance;
#endif

            FencedBuffer* mFencedB;   // current buffer
            android::Vector<FencedBuffer> mBuffers;

            bool mBuffersUpdatedThisFrame;
            bool mBuffersFilledAtLeastOnce;

        public:

            BufferSet(uint32_t width, uint32_t height, uint32_t format, int32_t numBuffers = -1,
                uint32_t usage = GRALLOC_USAGE_HW_COMPOSER | GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER);
            ~BufferSet();

            android::sp<android::GraphicBuffer> Get();
            buffer_handle_t GetHandle();
            buffer_handle_t GetNextBuffer();
            bool NeedsUpdating();
            bool BuffersFilledAtLeastOnce();
            uint32_t& GetInstanceParam();
            bool SetNextBufferInstance(uint32_t index);
            void AdvanceTimestamp(uint64_t delta);
            void PostFrame(int fenceFd);

            void SetReleaseFence(int fenceFd);
            int WaitReleaseFence(uint32_t timeoutMs, const android::String8& str);
            void CloseAllFences();

            uint32_t GetWidth();
            uint32_t GetHeight();

            void SetProtectionState(bool encrypted);
            void SetProtectionState(bool encrypted, uint32_t sessionId, uint32_t instance);

            // Number of buffers so far created
            static uint32_t GetBufferCount();

    };

    class BufferSetPtr : public android::sp<Hwch::BufferSet>
    {
        public:
             virtual ~BufferSetPtr();
             BufferSetPtr& operator=(android::sp<Hwch::BufferSet> rhs);
    };
};

inline uint32_t Hwch::BufferSet::GetWidth()
{
    return mWidth;
}

inline uint32_t Hwch::BufferSet::GetHeight()
{
    return mHeight;
}

inline bool Hwch::BufferSet::BuffersFilledAtLeastOnce()
{
    if (!mBuffersFilledAtLeastOnce)
    {
        mBuffersFilledAtLeastOnce = true;
        return false;
    }
    else
    {
        return true;
    }
}

#endif // __HwchBufferSet_h__
