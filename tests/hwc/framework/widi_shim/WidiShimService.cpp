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
 * @file    WidiShimService.cpp
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    9th October 2014
 * @brief   Implementation of the Widi service shim.
 *
 * @details This module implements the Widi service shim.
 *
 *****************************************************************************/

#include <binder/IServiceManager.h>

#include "HwcTestLog.h"
#include "IFrameServer.h"

#include "WidiShimService.h"
#include "WidiShimFrameListener.h"
#include "WidiShimFrameTypeChangeListener.h"

/** Instantiates the Shim service */
void WidiShimService::instantiate(HwcTestState *testState)
{
    HWCLOGI_COND(eLogWidi, "%s: WIDI called", __func__);

    // Get a strong pointer to the service manager
    android::sp<android::IServiceManager> sm(realServiceManager());
    if (sm == NULL)
    {
        HWCERROR(eCheckWidiBind, "%s: WIDI failed to get service manager", __func__);
        return;
    }

    android::sp<IBinder> hwc_widi = sm->checkService(android::String16(HWCVAL_WIDI_SERVICE_NAME));
    if (hwc_widi != NULL)
    {
        HWCERROR(eCheckWidiBind, "%s: WIDI service is already running as: %s", __func__, HWCVAL_WIDI_SERVICE_NAME);
        return;
    }

    // Register our own service (as if we were the HWC)
    if (sm->addService(android::String16(HWCVAL_WIDI_SERVICE_NAME), new WidiShimService(sm, testState)))
    {
        HWCERROR(eCheckWidiBind, "%s: WIDI could not add service as %s", __func__, HWCVAL_WIDI_SERVICE_NAME);
    }

    HWCLOGI("%s: successfully registered WIDI shim service as: %s", __func__, HWCVAL_WIDI_SERVICE_NAME);
}

/** Implements the start function - this is called when a screen cast request
    is made to the Intel WIDI stack */
android::status_t WidiShimService::start(android::sp<IFrameTypeChangeListener> typeChangeListener,
                                         bool disableExtVideoMode)
{
    HWCLOGD_COND(eLogWidi, "%s: WIDI called", __func__);

    // Connect to the real service
    mpHwcWidiService = android::interface_cast<IFrameServer>(
        mpSm->getService(android::String16(HWCVAL_WIDI_REAL_SERVICE_NAME)));
    if (mpHwcWidiService == NULL)
    {
        HWCERROR(eCheckWidiBind, "%s: failed to connect to 'real' service: " HWCVAL_WIDI_REAL_SERVICE_NAME, __func__);
        return android::FAILED_TRANSACTION;
    }

    // Get a pointer to the test kernel. Note, this has to be done now (and not in the constructor)
    // as HwcTestState::UseDRM (called after system initialisation) will allocate a new test kernel
    // in DRM enabled systems.
    if (mpTestState)
    {
        mpTestKernel = mpTestState->GetTestKernel();
    }
    else
    {
        HWCERROR(eCheckInternalError, "%s: test state pointer (mpTestState) is NULL.", __func__);
        return android::FAILED_TRANSACTION;
    }

    // Cache two flags (in the test kernel) to say that Widi has been enabled and whether extended
    // mode has been disabled. This makes this information available to the checks.
    if (mpTestKernel)
    {
        mpTestKernel->setWidiEnabled(true);
        mpTestKernel->setExtVideoModeDisable(disableExtVideoMode);
    }
    else
    {
        HWCERROR(eCheckInternalError, "%s: test kernel pointer (mpTestKernel) is NULL.", __func__);
        return android::FAILED_TRANSACTION;
    }

    // Reset the dropped frame counting state (if the frame listener exists).
    if (mpFrameListener.get())
    {
        mpFrameListener->resetLastFrame();
    }

    // Instantiate the FrameTypeChangeListener.
    mpFrameTypeChangeListener = new WidiShimFrameTypeChangeListener(typeChangeListener,
        mpFrameListener, mpTestKernel);
    if (!mpFrameTypeChangeListener.get())
    {
        HWCERROR(eCheckInternalError, "%s: test kernel pointer (mpTestKernel) is NULL.", __func__);
        return android::FAILED_TRANSACTION;
    }

    return mpHwcWidiService->start(mpFrameTypeChangeListener, disableExtVideoMode);
}

/** Implements the stop function which is called when the user stops casting */
android::status_t WidiShimService::stop(bool isConnected)
{
    HWCLOGD_COND(eLogWidi, "%s: WIDI called", __func__);

    // Reset the statistics
    if (!mpFrameListener.get() || !mpFrameTypeChangeListener.get())
    {
        HWCLOGW("%s: invalid frame listener or frame type change pointer.", __func__);
        return android::FAILED_TRANSACTION;
    }
    mpFrameListener->resetLastFrame();
    mpFrameListener->resetFrameCount();
    mpFrameTypeChangeListener->resetTypeChangeCounts();
    mSetResolutionCount = 0;
    mFrameCountForPreviousSetResolution = 0;
    mpFrameListener = NULL;

    if (mpTestKernel)
    {
        mpTestKernel->setWidiEnabled(false);
    }

    if (mpHwcWidiService != NULL)
    {
        return mpHwcWidiService->stop(isConnected);
    }

    return android::FAILED_TRANSACTION;
}

/** This function 'negotiates' a frame resolution and passes a pointer to the HWC
    for the frame listener i.e. the pipe for the HWC to send frames to the WIDI stack */
android::status_t WidiShimService::setResolution(const FrameProcessingPolicy& policy,
                                                 android::sp<IFrameListener> frameListener)
{
    HWCLOGD_COND(eLogWidi, "%s: WIDI Frame processing policy: scaledWidth: %d "
                           "scaledHeight: %d xdpi: %d ydpi: %d refresh: %d\n",
                           __func__, policy.scaledWidth, policy.scaledHeight,
                           policy.xdpi, policy.ydpi, policy.refresh);

    // Signal in the test kernel that we have seen a setResolution message
    // (which should be followed by a 'BufferInfo' notification).
    mpTestKernel->IncNumSetResolutions();
    mpTestKernel->setBufferInfoRequired(true);

    // Detect excessive numbers of setResolution messages. This can be caused (for example)
    // by cycles of erroneous FrameTypeChange messages triggering setResolution responses.
    if (mpFrameListener.get())
    {
        int32_t current_frame_count = mpFrameListener->getFrameCount();

        if ((current_frame_count - mFrameCountForPreviousSetResolution) >= mMaxSetResolutionWindow)
        {
            mSetResolutionCount = 0;
        }
        else
        {
            // Update the number of set resolution messages we have seen
            ++mSetResolutionCount;
            mFrameCountForPreviousSetResolution = current_frame_count;

            if (mpTestState)
            {
                mMaxSetResolutions = mpTestState->GetMaxSetResolutions();
            }
            else
            {
                HWCERROR(eCheckInternalError, "%s: test state pointer (mpTestState) is NULL.", __func__);
                return android::FAILED_TRANSACTION;
            }

            if (mSetResolutionCount > mMaxSetResolutions)
            {
                HWCERROR(eCheckWidiExcessiveSetResolutions,
                    "Seen %d SetResolution messages in %d frames. " \
                    "Excessive SetResolutions or potential cycle detected.",
                    mSetResolutionCount, mMaxSetResolutionWindow);
            }
        }
    }

    if (mpHwcWidiService != NULL)
    {
        if (mpFrameListener.get() == NULL)
        {
            // Instantiate the FrameListener.
            mpFrameListener = new WidiShimFrameListener(frameListener, mpTestKernel);
        }

        return mpHwcWidiService->setResolution(policy, mpFrameListener);
    }

    return android::FAILED_TRANSACTION;
}

#if LIBHWCWIDI_USES_ORIGINAL_MCG_API
/** Indicates that a buffer has been returned by the WIDI stack */
android::status_t WidiShimService::notifyBufferReturned(int index)
{
    HWCLOGD_COND(eLogWidi, "%s: WIDI called", __func__);

    if (mpHwcWidiService != NULL)
    {
        return mpHwcWidiService->notifyBufferReturned(index);
    }

    return android::FAILED_TRANSACTION;
}

android::status_t notifyBufferReturned(buffer_handle_t handle)
{
    HWCLOGD_COND(eLogWidi, "%s: WIDI called", __func__);

    if (mpHwcWidiService != NULL)
    {
        return mpHwcWidiService->notifyBufferReturned(handle);
    }

    return android::FAILED_TRANSACTION;
}
#endif
