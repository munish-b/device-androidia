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

#include "HwchWidi.h"

android::status_t Hwch::Widi::WidiFrameTypeChangeListener::frameTypeChanged(
    const FrameInfo& frameInfo)
{
    HWCLOGD_COND(eLogWidi, "%s: Harness Widi Frame Type Change Listener: frame type "
    "changed called for frame of type: %s", __func__,
    frameInfo.frameType == HWC_FRAMETYPE_NOTHING ? "nothing - SurfaceFlinger provides frames" :
    frameInfo.frameType == HWC_FRAMETYPE_FRAME_BUFFER ? "frame buffer" :
    frameInfo.frameType == HWC_FRAMETYPE_VIDEO ? "video" :
    frameInfo.frameType == HWC_FRAMETYPE_INCOMING_CALL ? "incoming call" : "unknown");

    HWCLOGD_COND(eLogWidi, "%s: Harness Widi Frame Type Change Listener: re-sending "
    "setResolution", __func__);

    mWidi.setResolution(mSystem.GetWirelessDisplayFrameProcessingPolicy());

    return android::NO_ERROR;
}

android::status_t Hwch::Widi::WidiFrameTypeChangeListener::bufferInfoChanged(
    const FrameInfo& frameInfo)
{
    HWCLOGD_COND(eLogWidi, "%s: Harness Widi Frame Type Change Listener: buffer info changed for "
    "frame of type: %s", __func__,
    frameInfo.frameType == HWC_FRAMETYPE_NOTHING ? "nothing - SurfaceFlinger provides frames" :
    frameInfo.frameType == HWC_FRAMETYPE_FRAME_BUFFER ? "frame buffer" :
    frameInfo.frameType == HWC_FRAMETYPE_VIDEO ? "video" :
    frameInfo.frameType == HWC_FRAMETYPE_INCOMING_CALL ? "incoming call" : "unknown");

    return android::NO_ERROR;
}

android::status_t Hwch::Widi::WidiFrameTypeChangeListener::shutdownVideo()
{
    HWCLOGD_COND(eLogWidi, "%s: Harness Widi Frame Type Change Listener: shutdown "
    "video called", __func__);

    return android::NO_ERROR;
}
