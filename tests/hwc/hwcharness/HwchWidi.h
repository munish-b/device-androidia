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

#ifndef __HwchWidi_h__
#define __HwchWidi_h__

#include <utils/RefBase.h>

#include "IFrameServer.h"
#include "IFrameTypeChangeListener.h"

#include "HwcTestState.h"
#include "HwchSystem.h"
#include "HwchWidiReleaseFencePool.h"

namespace Hwch
{
    class Widi : public android::RefBase
    {
        class WidiFrameListener : public BnFrameListener
        {
            WidiReleaseFencePool mReleaseFencePool;

            virtual android::status_t onFramePrepare(int64_t renderTimestamp,
                int64_t mediaTimestamp);

            virtual android::status_t onFrameReady( native_handle const* handle,
                int64_t renderTimestamp, int64_t mediaTimestamp, int acquireFenceFd,
                int* releaseFenceFd);

            virtual android::status_t onFrameReady(int32_t handle,
                HWCBufferHandleType handleType, int64_t renderTimestamp,
                int64_t mediaTimestamp);

            virtual android::status_t onFrameReady(const buffer_handle_t handle, HWCBufferHandleType handleType,
                int64_t renderTimestamp, int64_t mediaTimestamp);

        };

        class WidiFrameTypeChangeListener : public BnFrameTypeChangeListener
        {
            Hwch::Widi& mWidi;
            Hwch::System& mSystem;

            public:

            WidiFrameTypeChangeListener(Hwch::Widi& widi, Hwch::System& system)
                : mWidi(widi), mSystem(system) {}
            virtual ~WidiFrameTypeChangeListener() = default;

            virtual android::status_t frameTypeChanged(const FrameInfo& frameInfo);

            virtual android::status_t bufferInfoChanged(const FrameInfo& frameInfo);

            virtual android::status_t shutdownVideo();
        };

        bool mConnected = false;

        android::sp<IFrameServer> mpHwcWidiService;
        android::sp<WidiFrameListener> mpHwcWidiFrameListener;
        android::sp<android::IServiceManager> mpSm;
        Hwch::System& mSystem;
        static Widi* mInstance;

        public:

        /* Class design - big 5 plus destructor */
        Widi();
        Widi(const Widi& rhs) = delete;
        Widi(Widi&& rhs) = delete;
        Widi& operator=(const Widi& rhs) = delete;
        Widi& operator=(Widi&& rhs) = delete;
        virtual ~Widi() = default;

        android::status_t start(bool disableExtVideoMode);
        android::status_t stop(bool isConnected);
        android::status_t setResolution(const FrameProcessingPolicy& policy);

        // Widi connect, disconnect and 'is connected' functions
        android::status_t WidiConnect();
        android::status_t WidiConnect(uint32_t width, uint32_t height);
        android::status_t WidiConnect(const struct FrameProcessingPolicy& policy);
        android::status_t WidiDisconnect();

        bool WidiIsConnected()
        {
            return mConnected;
        }

        static Widi& getInstance();
    };
}

#endif // __HwchWidi_h__
