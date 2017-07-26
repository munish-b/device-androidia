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

#include <binder/IServiceManager.h>

#include "HwcTestState.h"

#include "HwchSystem.h"
#include "HwchWidi.h"

/** Default Constructor */
Hwch::Widi::Widi() :
    mSystem(Hwch::System::getInstance())
{ }

/** Implements the start function - this is called when a screen cast request is made */
android::status_t Hwch::Widi::start(bool disableExtVideoMode)
{
    HWCLOGD_COND(eLogWidi, "%s: WIDI called", __func__);

    // Get a strong pointer to the service manager
    mpSm = realServiceManager();
    if (mpSm == NULL)
    {
        HWCERROR(eCheckWidiBind, "%s: WIDI failed to get service manager", __func__);
        return android::FAILED_TRANSACTION;
    }

    // Connect to the Widi service
    mpHwcWidiService = android::interface_cast<IFrameServer>(
        mpSm->getService(android::String16(HWCVAL_WIDI_SERVICE_NAME)));
    if (mpHwcWidiService == NULL)
    {
        HWCERROR(eCheckWidiBind, "%s: failed to connect to service: " HWCVAL_WIDI_SERVICE_NAME, __func__);
        return android::FAILED_TRANSACTION;
    }

    return mpHwcWidiService->start(new WidiFrameTypeChangeListener(*this, mSystem),
        disableExtVideoMode);
}

/** Implements the stop function which is called when the user stops casting */
android::status_t Hwch::Widi::stop(bool isConnected)
{
    HWCLOGD_COND(eLogWidi, "%s: WIDI called", __func__);

    if (mpHwcWidiService != NULL)
    {
        return mpHwcWidiService->stop(isConnected);
    }

    return android::FAILED_TRANSACTION;
}

/** This function 'negotiates' a frame resolution and passes a pointer to the
    HWC for the frame listener i.e. the pipe for HWC to send frames to the WIDI stack */
android::status_t Hwch::Widi::setResolution(const FrameProcessingPolicy& policy)
{
    HWCLOGD_COND(eLogWidi, "%s: WIDI called", __func__);

    if (mpHwcWidiService != NULL)
    {
        if (mpHwcWidiFrameListener == NULL)
        {
            mpHwcWidiFrameListener = new WidiFrameListener();
            if (mpHwcWidiFrameListener == NULL)
            {
                HWCLOGD("Could not allocate a new WidiFrameListener");
            }
        }

        return mpHwcWidiService->setResolution(policy, mpHwcWidiFrameListener.get());
    }

    return android::FAILED_TRANSACTION;
}

// Connect to the Widi service in the HWC.
android::status_t Hwch::Widi::WidiConnect(const struct FrameProcessingPolicy& policy)
{
    // Connect to the Widi stack.
    //
    // Note: the Widi subsystem must still be configured on the command line with the
    // -widi parameter (see HwcHarness.cpp for details).
    android::status_t ret = android::NO_ERROR;
    if (mInstance)
    {
        HWCLOGI("Calling Widi 'start' function\n");
        ret = mInstance->start(false);
        if (ret != android::NO_ERROR)
        {
            return ret;
        }

        HWCLOGI("Making Widi 'set resolution' call \n");
        ret = mInstance->setResolution(policy);
    }
    else
    {
        HWCERROR(eCheckInternalError, "Could not allocate Widi object for Widi test!");
    }

    if (ret == android::NO_ERROR)
    {
        mConnected = true;
    }

    // Enable the display
    Hwch::Display& widi_display = mSystem.GetDisplay(HWCVAL_DISPLAY_ID_WIDI_VIRTUAL);
    widi_display.SetConnected(true);

    return ret;
}

// Add an overload to lookup and use the default frame processing policy.
android::status_t Hwch::Widi::WidiConnect()
{
    Hwch::System& system = Hwch::System::getInstance();
    return WidiConnect(system.GetWirelessDisplayFrameProcessingPolicy());
}

// Add an overload when we want to specify width/height only
android::status_t Hwch::Widi::WidiConnect(uint32_t width, uint32_t height)
{
    Hwch::System& system = Hwch::System::getInstance();

    // Redefine the scaled height and width - keep the existing refresh rate
    struct FrameProcessingPolicy& frame_processing_policy =
        system.GetWirelessDisplayFrameProcessingPolicy();

    system.SetWirelessDisplayFrameProcessingPolicy(
        width, height, frame_processing_policy.refresh);

    return WidiConnect(frame_processing_policy);
}

// Disconnect from the HWC Widi service.
android::status_t Hwch::Widi::WidiDisconnect()
{
    HWCLOGI("Calling Widi 'stop' function\n");

    android::status_t ret = mInstance ? mInstance->stop(true)
        : (android::UNKNOWN_ERROR);

    if (ret == android::NO_ERROR)
    {
        mConnected = false;
    }

    // Disable the display
    Hwch::Display& widi_display = mSystem.GetDisplay(HWCVAL_DISPLAY_ID_WIDI_VIRTUAL);
    widi_display.SetConnected(false);

    return ret;
}

// Implement a singleton instance of Widi in the same manner as HwchSystem does.
Hwch::Widi* Hwch::Widi::mInstance = 0;

Hwch::Widi& Hwch::Widi::getInstance()
{
    if (mInstance == 0)
    {
        mInstance = new Hwch::Widi();
    }
    return *mInstance;
}
