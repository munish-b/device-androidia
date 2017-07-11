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
 * @file    WidiShimFrameTypeChangeListener.h
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    9th March 2015
 * @brief   Shim support for the Widi Type Change and Buffer Info interfaces
 *
 * @details This header adds support for the 'frame type change' and 'buffer
 *          info' interfaces.
 *
 *****************************************************************************/

#ifndef __WidiShimFrameTypeChangeListener_h__
#define __WidiShimFrameTypeChangeListener_h__

#include "IFrameTypeChangeListener.h"
#include "HwcTestKernel.h"
#include "WidiShimFrameListener.h"

class WidiShimFrameTypeChangeListener : public BnFrameTypeChangeListener
{

public:

    /* Class design - big 5 plus destructor */
    WidiShimFrameTypeChangeListener() = delete;
    WidiShimFrameTypeChangeListener(const WidiShimFrameTypeChangeListener& rhs) = delete;
    WidiShimFrameTypeChangeListener(WidiShimFrameTypeChangeListener&& rhs) = delete;
    WidiShimFrameTypeChangeListener& operator=(const WidiShimFrameTypeChangeListener& rhs) = delete;
    WidiShimFrameTypeChangeListener& operator=(WidiShimFrameTypeChangeListener&& rhs) = delete;
    virtual ~WidiShimFrameTypeChangeListener() = default;

    /** Main user constructor */
    WidiShimFrameTypeChangeListener(android::sp<IFrameTypeChangeListener> realListener,
        android::sp<WidiShimFrameListener> frameListener, HwcTestKernel *testKernel) :
        mRealListener(realListener), mpFrameListener(frameListener), mpTestKernel(testKernel) {}

    /** Overrides implemented in WidiShimFrameTypeChangeListener.cpp */
    virtual android::status_t frameTypeChanged(const FrameInfo& frameInfo) override;
    virtual android::status_t bufferInfoChanged(const FrameInfo& frameInfo) override;

    virtual android::status_t shutdownVideo() override
    {
        HWCLOGD_COND(eLogWidi, "%s: WIDI unsupported function called", __func__);

        return android::OK;
    };

    /** Method to reset the statistics (called on disconnection) */
    void resetTypeChangeCounts()
    {
        mFrameTypeChangeCount = 0;
        mBufferInfoCount = 0;
    }

private:

    /** Pointer to the 'real' listener (running as part of HWC) */
    android::sp<IFrameTypeChangeListener> mRealListener;

    /** Store a pointer to the frame listener so that we can access the frame count */
    android::sp<WidiShimFrameListener> mpFrameListener;

    /** Cache a pointer to the test kernel */
    HwcTestKernel *mpTestKernel = NULL;

    /** Store the last FrameInfo so that we can detect duplicate FrameTypeChanged and
        BufferInfoChanged messages. */
    FrameInfo mLastFrameInfo;

    /** Store the frame index for the last frame type change message */
    int32_t mLastFrameTypeChangeIndex = 0;

    /** Member variables to count number of FrameTypeChange and BufferInfo messages */
    uint32_t mFrameTypeChangeCount = 0;
    uint32_t mBufferInfoCount = 0;
};

#endif // __WidiShimFrameTypeChangeListener_h__
