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
 * @file    Widi.h
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    5th January 2015
 * @brief   Header for the Harness Widi support.
 *
 * @details Top-level header for the Widi support in the Harness. This class
 *          performs two functions. Firstly, it provides the necessary infra-
 *          structure to connect to and manipulate the Widi service (which is
 *          running either as part of the shims or as part of the HWC) and
 *          secondly, it provides the 'listener' entities for receiving frames
 *          and 'frame type change' notifications from the HWC.
 *
 *****************************************************************************/

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
