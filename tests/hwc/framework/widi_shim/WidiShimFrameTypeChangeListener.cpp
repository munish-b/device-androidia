/****************************************************************************
 *
 * Copyright (c) Intel Corporation (2015).
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
 * @file    WidiShimFrameTypeChangeListener.cpp
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    1st April 2015
 * @brief   Shim support for the Widi Type Change and Buffer Info interfaces
 *
 * @details This file implements support for the 'frame type change' and 'buffer
 *          info' interfaces.
 *
 *****************************************************************************/

#include "IFrameTypeChangeListener.h"
#include "HwcTestKernel.h"
#include "WidiShimFrameTypeChangeListener.h"

android::status_t WidiShimFrameTypeChangeListener::frameTypeChanged(const FrameInfo& frameInfo)
{
    HWCLOGD_COND(eLogWidi, "%s: WIDI frameInfo seen", __func__);

    HwcTestState *pTestState = HwcTestState::getInstance();

    // Export (and increment) the number of FrameTypeChange messages that have been received
    if (pTestState)
    {
        pTestState->SetWirelessFrameTypeChangeCount(++mFrameTypeChangeCount);
    }
    else
    {
        HWCERROR(eCheckInternalError, "%s: WIDI listener detected that pTestState is invalid!", __func__);
    }

    // Test whether this is a duplicate FrameTypeChange message
    //
    // Note: FrameInfo does not implement an equality operator. Also, FrameInfo
    // is a Plain Old Data Structure (PODS) (with no indirection) so its simpler
    // to just use a memcmp (for testing equality) rather than defining a global
    // overload. If this ever changes (e.g. FrameInfo contains pointer fields),
    // then a global overload would be required.
    if (memcmp(&frameInfo, &mLastFrameInfo, sizeof(FrameInfo)) == 0)
    {
        int32_t mFrameTypeChangeIndexDifference = 0;
        if (mpFrameListener.get())
        {
            int32_t mCurrentFrameTypeChangeIndex = mpFrameListener->getFrameCount();
            mFrameTypeChangeIndexDifference = mLastFrameTypeChangeIndex - mCurrentFrameTypeChangeIndex;
        }

        HWCCHECK(eCheckWidiDuplicateFrameInfo);
        if (mpFrameListener.get())
        {
            HWCCHECK(eCheckWidiConsecutiveDuplicateFrameInfo);
            HWCCHECK(eCheckWidiInterleavedDuplicateFrameInfo);
            HWCCHECK(eCheckWidiNonConsecutiveDuplicateFrameInfo);

            if (mFrameTypeChangeIndexDifference == 0)
            {
                HWCERROR(eCheckWidiConsecutiveDuplicateFrameInfo, "Consecutive duplicate FrameTypeChange message "
                    "detected.");
            }
            else if (mFrameTypeChangeIndexDifference == 1)
            {
                HWCERROR(eCheckWidiInterleavedDuplicateFrameInfo, "Duplicate FrameTypeChange message was "
                    "interleaved with a regular frame");
            }
            else if (mFrameTypeChangeIndexDifference > 1)
            {
                HWCERROR(eCheckWidiNonConsecutiveDuplicateFrameInfo, "Non-consecutive duplicate FrameTypeChange message. "
                    "Last duplicate was %d frames ago", mFrameTypeChangeIndexDifference);
            }
            else
            {
                HWCERROR(eCheckWidiDuplicateFrameInfo, "Duplicate FrameTypeChange message detected "
                    "(with negative index difference).");
            }
        }
    }
    else
    {
        // Not a duplicate - store the frame count for this FrameTypeChange message.
        if (mpFrameListener.get())
        {
            mLastFrameTypeChangeIndex = mpFrameListener->getFrameCount();
        }

        mLastFrameInfo = frameInfo;
    }

    // Signal the new frame type to the test kernel
    if (mpTestKernel)
    {
        HWCLOGD_COND(eLogWidi, "%s: Widi Shim Frame Type Change Listener: frame type "
            "changed called for frame of type: %s", __func__,
            frameInfo.frameType == HWC_FRAMETYPE_NOTHING ? "nothing - SurfaceFlinger provides frames" :
            frameInfo.frameType == HWC_FRAMETYPE_FRAME_BUFFER ? "frame buffer" :
            frameInfo.frameType == HWC_FRAMETYPE_VIDEO ? "video" :
            frameInfo.frameType == HWC_FRAMETYPE_INCOMING_CALL ? "incoming call" : "unknown");

        if (mpTestKernel)
        {
            mpTestKernel->setExpectedWidiFrameType(frameInfo.frameType);
        }
        else
        {
            HWCERROR(eCheckInternalError, "Can not set expected Widi frame type - pointer to test kernel is null!");
        }
    }
    else
    {
        HWCERROR(eCheckInternalError, "%s: pointer to test kernel is NULL! Not all checks run!", __func__);
    }

    if (mRealListener != NULL)
    {
        mRealListener->frameTypeChanged(frameInfo);
    }

    return android::OK;
}

android::status_t WidiShimFrameTypeChangeListener::bufferInfoChanged(const FrameInfo& frameInfo)
{
    HWCLOGD_COND(eLogWidi, "%s: WIDI bufferInfoChanged seen", __func__);

    HwcTestState *pTestState = HwcTestState::getInstance();

    // Export (and increment) the number of BufferInfo messages that have been received
    if (pTestState)
    {
        pTestState->SetWirelessBufferInfoCount(++mBufferInfoCount);
    }
    else
    {
        HWCERROR(eCheckInternalError, "%s: WIDI listener detected that pTestState is invalid!", __func__);
    }

    // Test whether this is a duplicate BufferInfo message
    //
    // The difference between a 'FrameTypeChanged' call and a 'bufferInfoChanged' call is the
    // fields that are updated in the FrameInfo structure. For a FrameTypeChanged update, the
    // updated fields are: the frame type, content height, content width and FPS. For a
    // bufferInfoChanged call, the updated fields are: buffer width, buffer height, crop, luma
    // and chroma.
    //
    // See frameTypeChanged (above) for a description of why a memcmp is sufficient here.
    HWCCHECK(eCheckWidiDuplicateFrameInfo);
    if (memcmp(&frameInfo, &mLastFrameInfo, sizeof(FrameInfo)) == 0)
    {
        HWCERROR(eCheckWidiDuplicateFrameInfo, "Detected duplicate FrameInfo in BufferInfo message");
    }
    else
    {
        mLastFrameInfo = frameInfo;
    }

    if (mpTestKernel)
    {
        mpTestKernel->setBufferInfoRequired(false);
    }
    else
    {
        HWCERROR(eCheckInternalError, "Can not set buffer info required - pointer to test kernel is null!");
    }

    if (mRealListener != NULL)
    {
        mRealListener->bufferInfoChanged(frameInfo);
    }

    return android::OK;
}
