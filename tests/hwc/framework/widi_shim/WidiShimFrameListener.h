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
 * @file    WidiShimFrameListener.h
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    22nd October 2014
 * @brief   Header for the Widi shim frame listener.
 *
 * @details This is the header for the Widi shim frame listener. This native
 *          binder class subclasses BnFrameListener, that is, the listener
 *          interface which receives frames into the Intel miracast stack (see
 *          in $ANDROID_TOP/vendor/intel/hardware/PRIVATE/widi/libhwcwidi). This
 *          class is instantiated in WidiShimService::setResolution and is passed
 *          to the 'real' Widi service running as part of the HWC.
 *
 *****************************************************************************/

#ifndef __WidiShimFrameListener_h__
#define __WidiShimFrameListener_h__

#include "IFrameServer.h"
#include "HwcTestKernel.h"

class WidiShimFrameListener : public BnFrameListener
{

public:

    /* Class design - big 5 plus destructor */
    WidiShimFrameListener() = delete;
    WidiShimFrameListener(const WidiShimFrameListener& rhs) = delete;
    WidiShimFrameListener(WidiShimFrameListener&& rhs) = delete;
    WidiShimFrameListener& operator=(const WidiShimFrameListener& rhs) = delete;
    WidiShimFrameListener& operator=(WidiShimFrameListener&& rhs) = delete;
    virtual ~WidiShimFrameListener() = default;

    /** Main user constructor */
    WidiShimFrameListener(android::sp<IFrameListener> realListener, HwcTestKernel *testKernel) :
        mRealListener(realListener), mpTestKernel(testKernel) {}

    /** We have to override onFramePrepare because it is declared pure virtual */
    virtual android::status_t onFramePrepare(int64_t renderTimestamp,
        int64_t mediaTimestamp) override
    {
        return mRealListener->onFramePrepare(renderTimestamp, mediaTimestamp);
    }

    /** Override onFrameReady to 'tee' into the flow of frames and pass them to the test kernel. */
    virtual android::status_t onFrameReady(const native_handle* handle,
                                           int64_t renderTimestamp, int64_t mediaTimestamp,
                                           int acquireFenceFd, int* releaseFenceFd) override;

    /** This overload is not called by the VPG HWC. This version is used by the IMG 'merryfield' HWC
        which is developed in Shanghai. Note, that we have to override it because it is pure virtual. */
    virtual android::status_t onFrameReady(int64_t handle, HWCBufferHandleType handleType,
                                           int64_t renderTimestamp, int64_t mediaTimestamp);

    virtual android::status_t onFrameReady(const buffer_handle_t handle, HWCBufferHandleType handleType,
                                           int64_t renderTimestamp, int64_t mediaTimestamp);

    /** Frame count accessor */
    int32_t getFrameCount() const
    {
        return mFrameCount;
    }

    /** Method to reset the frame count (called on disconnection) */
    void resetFrameCount()
    {
        mFrameCount = 0;
    }

    /** Method to reset the frame number of the last frame received (used during disconnection) */
    void resetLastFrame()
    {
        mLastHwcFrame = 0;
    }

private:

    /** Pointer to the 'real' listener (running as part of HWC) */
    android::sp<IFrameListener> mRealListener;

    /** Pointer to the test kernel */
    HwcTestKernel *mpTestKernel = NULL;

    /** Member variable to count frames. Note, that at 60 fps, this will take ~ 2 years to rollover. */
    uint32_t mFrameCount = 0;

    /** Stores the last value of mHwcFrame (see HwcTestKernel.cpp) */
    uint32_t mLastHwcFrame = 0;
};

#endif // __WidiShimFrameListener_h__
