/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
