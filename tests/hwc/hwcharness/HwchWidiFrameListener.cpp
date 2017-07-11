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
 * @file    WidiFrameListener.cpp
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    27th January 2015 (split from Widi)
 * @brief   Implementation for the Frame Listener class (i.e. the class where
 *          frames are ultimately delivered).
 *
 * @details This file implements the Frame Listener class. This is the harness
 *          endpoint for Widi frames.
 *
 *****************************************************************************/

#include <stdint.h>

#include "HwcTestState.h"
#include "HwchWidi.h"

#include "HwchLayerWindowed.h"

/** The onFramePrepare hook is not called by the HWCi or shims. However, we
    define it because it is declared pure virtual in the BnFrameListener interface */
android::status_t Hwch::Widi::WidiFrameListener::onFramePrepare(
    int64_t renderTimestamp, int64_t mediaTimestamp)
{
    HWCLOGD_COND(eLogWidi, "%s: Harness Widi frame listener received prepare call. "
        "Render timestamp: %" PRId64 " media timestamp: %" PRId64, __func__,
        renderTimestamp, mediaTimestamp);

    return android::NO_ERROR;
}

/** The onFrameReady hook is called by the HWC (or Widi shim). This is the frame end-point.
    We wait on a frame's acquire flag and then signal a release fence - either in-order (to
    simulate normal operation) or randomly (to simulate the behaviour of the Widi stack.

    See: WidiReleaseFencePool.h for more details. */
android::status_t Hwch::Widi::WidiFrameListener::onFrameReady(const native_handle* handle,
    int64_t renderTimestamp, int64_t mediaTimestamp, int acquireFenceFd, int* releaseFenceFd)
{
    HWCLOGD_COND(eLogWidi, "%s: Harness Widi frame listener received frame - handle: %p, "
        "renderTimestamp: %" PRId64 " mediaTimestamp: %" PRId64 " acquire fence: %d", __func__,
        handle, renderTimestamp, mediaTimestamp, acquireFenceFd);

    // Wait on the acquire fence to prevent the possibility of signalling the release fence
    // before the acquire
    if (acquireFenceFd > 0)
    {
        int32_t err = sync_wait(acquireFenceFd, 0);
        if (err < 0)
        {
            HWCLOGD_COND(eLogWidi, "%s: timeout expired for acquire fence");
        }
        CloseFence(acquireFenceFd);
    }

    *releaseFenceFd = mReleaseFencePool.GetReleaseFence();
    if (releaseFenceFd && (*releaseFenceFd == -1))
    {
        HWCLOGD_COND(eLogWidi, "%s: received release fence with fd -1!", __func__);
    }

    HWCLOGD_COND(eLogWidi, "%s: Harness Widi frame listener allocated release fence: %d",
        __func__, *releaseFenceFd);

    // Display the frame in the Widi visualisation window (if enabled)
    Hwch::System& system = Hwch::System::getInstance();
    if (system.IsWirelessWindowEnabled())
    {
        system.SetWirelessWindowedLayer(new HwchLayerWindowed(system.GetWirelessWindowWidth(),
            system.GetWirelessWindowHeight(), handle));
    }

    // Signal the next fence according to the fence release policy:
    //  - Sequential FIFO order (sanity check)
    //  - Random order
    //  - Retain oldest index for 'widi_retain_oldest' frames before signalling
    switch (Hwch::System::getInstance().GetWirelessFenceReleaseMode())
    {
        using FenceReleaseMode = Hwch::System::FenceReleaseMode;

        case FenceReleaseMode::eSequential:
            mReleaseFencePool.SignalSequentialIndex();
            break;
        case FenceReleaseMode::eRandom:
            mReleaseFencePool.SignalRandomIndex();
            break;
        case FenceReleaseMode::eRetainOldest:
            mReleaseFencePool.SignalOldestIndex();
            break;
    }

    return android::NO_ERROR;
}

/** Overload called by the IMG 'merryfield' HWC. Note, that we have to override it because
    it is pure virtual. */
android::status_t Hwch::Widi::WidiFrameListener::onFrameReady(int32_t handle,
    HWCBufferHandleType handleType, int64_t renderTimestamp, int64_t mediaTimestamp)
{
    HWCLOGD_COND(eLogWidi, "%s: Harness Widi frame listener received frame at unsupported entry"
        "point - CHECKS NOT RUN (handle: %p, renderTimestamp: %" PRId64 " mediaTimestamp: %" PRId64
        " Buffer type: %s)" PRId64, __func__, handle, renderTimestamp, mediaTimestamp,
        handleType == HWC_HANDLE_TYPE_GRALLOC ? "gralloc" :
        handleType == HWC_HANDLE_TYPE_KBUF ? "kernel" : "unknown");

    return android::NO_ERROR;
}

android::status_t Hwch::Widi::WidiFrameListener::onFrameReady(const buffer_handle_t handle, HWCBufferHandleType handleType,
                int64_t renderTimestamp, int64_t mediaTimestamp)
{
    HWCLOGD_COND(eLogWidi, "%s: Harness Widi frame listener received frame at second unsupported entry"
        "point - CHECKS NOT RUN (handle: %p, renderTimestamp: %" PRId64 " mediaTimestamp: %" PRId64
        " Buffer type: %s)" PRId64, __func__, handle, renderTimestamp, mediaTimestamp,
        handleType == HWC_HANDLE_TYPE_GRALLOC ? "gralloc" :
        handleType == HWC_HANDLE_TYPE_KBUF ? "kernel" : "unknown");

    return android::NO_ERROR;
}
