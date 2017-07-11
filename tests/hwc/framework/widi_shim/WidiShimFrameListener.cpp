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
 * @file    WidiShimFrameListener.cpp
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    1st April 2015
 * @brief   Implementation for the Widi shim frame listener.
 *
 * @details This is the implementation of the Widi shim frame listener. This native
 *          binder class subclasses BnFrameListener, that is, the listener
 *          interface which receives frames into the Intel miracast stack (see
 *          in $ANDROID_TOP/vendor/intel/hardware/PRIVATE/widi/libhwcwidi). This
 *          class is instantiated in WidiShimService::setResolution and is passed
 *          to the 'real' Widi service running as part of the HWC.
 *
 *****************************************************************************/

#include "IFrameServer.h"
#include "HwcTestKernel.h"
#include "HwcTestState.h"
#include "WidiShimFrameListener.h"

/** Override onFrameReady to 'tee' into the flow of frames and pass them to the test kernel. */
android::status_t WidiShimFrameListener::onFrameReady(const native_handle* handle,
    int64_t renderTimestamp, int64_t mediaTimestamp,
    int acquireFenceFd, int* releaseFenceFd)
{
    HwcTestState *pTestState = HwcTestState::getInstance();

    // Export (and increment) the frame count
    if (pTestState)
    {
        pTestState->SetWirelessFrameCount(++mFrameCount);
    }
    else
    {
        HWCERROR(eCheckInternalError, "%s: WIDI listener detected that pTestState is invalid!", __func__);
    }

    HWCLOGD_COND(eLogWidi, "%s: WIDI listener passing frame (handle: %p release fence: %d count: %d) "
        "to checks" , __func__, handle, (releaseFenceFd != NULL) ? *releaseFenceFd : 0, mFrameCount);

    // Implement dropped frame counting
    if (mpTestKernel)
    {
        uint32_t current_frame = mpTestKernel->GetHwcFrame(HWCVAL_VD_WIDI_DISPLAY_INDEX); // Get current frame number

        // mLastHwcFrame will be 0 just after connection. In which case, just update it for next time.
        if (mLastHwcFrame)
        {
            HwcTestCrtc* crtc = mpTestKernel->GetHwcTestCrtcByDisplayIx(HWCVAL_VD_WIDI_DISPLAY_INDEX);
            if (!crtc)
            {
                HWCERROR(eCheckInternalError, "%s: could not lookup crtc!", __func__);
            }
            else
            {
                // Calculate dropped frames as current frame (as seen by the test kernel) - last frame
                // number as seen by Widi. Decrement by one to prevent current frame from being counted
                // as dropped. Note, dropped_frames is a uint, so has to be guarded against rollover if
                // its zero.
                uint32_t dropped_frames = current_frame - mLastHwcFrame;
                if (dropped_frames > 0)
                {
                    crtc->AddDroppedFrames(--dropped_frames);
                }
                else
                {
                    // No dropped frames - reset consecutive count.
                    crtc->ResetConsecutiveDroppedFrames();
                }

                // Mirror the protected content status of the buffer into the plane
                ALOG_ASSERT(crtc->NumPlanes() == 1);

                DrmShimPlane* pPlane = crtc->GetPlane(0); // 0 indexed
                android::sp<DrmShimBuffer> buf = mpTestKernel->lookupDrmShimBuffer(handle);

                ALOG_ASSERT(pPlane);
                ALOG_ASSERT(buf != NULL);

                // Update the decryption state of the plane to match that of the buffer
                pPlane->SetDecrypt(buf->HasMediaDetailsEncrypted());
            }

        }
        mLastHwcFrame = current_frame;
        mpTestKernel->SetWidiLastFrame(mLastHwcFrame);

        // Send frame to checks
        mpTestKernel->checkWidiBuffer(handle);
    }
    else
    {
        HWCERROR(eCheckInternalError, "%s: WIDI listener detected that mpTestKernel is invalid!", __func__);
    }

    android::status_t ret = mRealListener->onFrameReady(
        handle, renderTimestamp, mediaTimestamp, acquireFenceFd, releaseFenceFd);
    if (ret != android::NO_ERROR)
    {
        HWCERROR(eCheckWidiOnFrameReadyError, "Call to onFrameReady returned error: %d", ret);
    }

    return ret;
}

/** This overload is not called by the VPG HWC. This version is used by the IMG 'merryfield' HWC
    which is developed in Shanghai. Note, that we have to override it because it is pure virtual. */
android::status_t WidiShimFrameListener::onFrameReady(int64_t handle, HWCBufferHandleType handleType,
    int64_t renderTimestamp, int64_t mediaTimestamp)
{
    HwcTestState *pTestState = HwcTestState::getInstance();

    // Export (and increment) the frame count
    if (pTestState)
    {
        pTestState->SetWirelessFrameCount(++mFrameCount);
    }
    else
    {
        HWCERROR(eCheckInternalError, "%s: WIDI listener detected that pTestState is invalid!", __func__);
    }

    HWCERROR(eCheckInternalError, "%s: WIDI listener saw frame (count: %d) on unsupported entry point - checks NOT run",
        __func__, mFrameCount);

    // Just pass the call through
    android::status_t ret = mRealListener->onFrameReady(
        (buffer_handle_t)handle, handleType, renderTimestamp, mediaTimestamp);
    if (ret != android::NO_ERROR)
    {
        HWCERROR(eCheckWidiOnFrameReadyError, "Unsupported call to onFrameReady returned error: %d", ret);
    }

    return ret;
}

android::status_t WidiShimFrameListener::onFrameReady(const buffer_handle_t handle, HWCBufferHandleType handleType,
    int64_t renderTimestamp, int64_t mediaTimestamp)
{
    HwcTestState *pTestState = HwcTestState::getInstance();

    // Export (and increment) the frame count
    if (pTestState)
    {
        pTestState->SetWirelessFrameCount(++mFrameCount);
    }
    else
    {
        HWCERROR(eCheckInternalError, "%s: WIDI listener detected that mpTestState is invalid!", __func__);
    }

    HWCERROR(eCheckInternalError, "%s: WIDI listener saw frame (count: %d) on unsupported entry point - checks NOT run",
        mFrameCount, __func__, mFrameCount);

    // Just pass the call through
    android::status_t ret = mRealListener->onFrameReady(handle, handleType, renderTimestamp, mediaTimestamp);
    if (ret != android::NO_ERROR)
    {
        HWCERROR(eCheckWidiOnFrameReadyError, "Unsupported call to onFrameReady returned error: %d", ret);
    }

    return ret;
}
