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
 * @file    WidiFrameTypeChangeListener.h
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    27th January 2015 (split from Widi)
 * @brief   Implementation for the Widi 'Frame Type Change' listener.
 *
 * @details This is the implementation for the 'Frame Type Change' listener
 *          i.e. the end-point in the harness to which the HWC sends (via the
 *          shims - if they are installed) frame type change notifications.
 *          These notifications indicate whether the HWC is sending video to
 *          the Widi stack.
 *
 *****************************************************************************/

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
