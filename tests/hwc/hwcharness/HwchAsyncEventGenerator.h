/****************************************************************************
*
* Copyright (c) Intel Corporation (2014).
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
* File Name:            HwchAsyncEventGenerator.h
*
* Description:          Generates suspend/resume, blank/unblank,
*                       ESD recovery and hot plug events with an option
*                       for them to take place asynchronously after a delay.
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#ifndef __HwchAsyncEventGenerator_h__
#define __HwchAsyncEventGenerator_h__

#include "EventMultiThread.h"
#include <ui/GraphicBuffer.h>
#include <utils/StrongPointer.h>
#include "HwchDefs.h"
#include "IDisplayModeControl.h"
#include "IVideoControl.h"
#include "HwchDisplay.h"
#include "HwcvalWatchdog.h"

#ifdef HWCVAL_BUILD_HWCSERVICE_API
#include "HwcServiceApi.h"
#endif

namespace Hwch
{
    class Interface;

    class AsyncEvent
    {
    public:
        static android::String8 EventName(uint32_t eventType);

    public:
        // Event type bitmask
        // Some events can be combined (e.g. blank and suspend); other combinations
        // do not make sense (suspend and resume) and hence won't take place.
        enum Type
        {
            // Send Blank/unblank via HWC interface
            eBlank = 1,
            eUnblank = 2,

            // Suspend/resume power
            eSuspend = 4,
            eResume = 8,

            // Widi
            eWidiConnect = 0x80,
            eWidiDisconnect = 0x100,
            eWidiFencePolicy = 0x200,

            // Wireless docking mode
            eWirelessDockingEntry = 0x400,
            eWirelessDockingExit = 0x800,

            // Display mode set
            eModeSet = 0x1000,
            eModeClear = 0x2000,
            eSetVideoOptimizationMode = 0x4000,

            // KERNEL EVENTS
            // ESD recovery
            eESDRecovery = 0x10000000,

            // Hotplug/unplug
            eHotPlug = 0x20000000,
            eHotUnplug = 0x40000000
        };

        // Types(s) of display to hot plug
        static const uint32_t cFixedDisplay;
        static const uint32_t cRemovableDisplay;
        static const uint32_t cAllDisplays;

        // Repeat data class for events
        // (especially kernel events)
        class RepeatData : public android::RefBase
        {
        public:
            RepeatData(uint32_t burstLength=1, uint32_t delayUs=0) :
                mBurstLength(burstLength),
                mDelayUs(delayUs)
            {
            }

            virtual ~RepeatData()
            {
            }

            uint32_t mBurstLength;
            uint32_t mDelayUs;
        };

        // Generic additional data class for events
        class Data : public android::RefBase
        {
        public:
            Data()
            {
            }

            virtual ~Data()
            {
            }
        };

        class HotPlugEventData : public Data
        {
        public:
            HotPlugEventData(uint32_t displayTypes);
            virtual ~HotPlugEventData();

            uint32_t mDisplayTypes;
        };

        class ModeChangeEventData : public Data
        {
        public:
            ModeChangeEventData(uint32_t displayIx,
                                android::sp<::intel::ufo::hwc::services::IDisplayModeControl> dispModeControl,
                                const Display::Mode& mode);
            virtual ~ModeChangeEventData();

            uint32_t mDisplayIx;
            android::sp<::intel::ufo::hwc::services::IDisplayModeControl> mDispModeControl;
            Display::Mode mMode;
        };

        class VideoOptimizationModeData : public Data
        {
        public:
            VideoOptimizationModeData(android::sp<::intel::ufo::hwc::services::IVideoControl> videoControl,
                                const Display::VideoOptimizationMode videoOptimizationMode);
            virtual ~VideoOptimizationModeData();

            android::sp<::intel::ufo::hwc::services::IVideoControl> mVideoControl;
            Display::VideoOptimizationMode mVideoOptimizationMode;
        };

        class WidiConnectEventData : public Data
        {
        public:
            WidiConnectEventData(uint32_t width, uint32_t height) :
                mWidth(width), mHeight(height) {};
            virtual ~WidiConnectEventData() {};

            uint32_t mWidth = 0;
            uint32_t mHeight = 0;
        };

        class WidiFencePolicyEventData : public Data
        {
        public:
            WidiFencePolicyEventData(uint32_t mode, uint32_t retain_oldest) :
                mFencePolicyMode(mode), mRetainOldest(retain_oldest) {}
            virtual ~WidiFencePolicyEventData() {};

            uint32_t mFencePolicyMode = 0;
            uint32_t mRetainOldest = 0;
        };

        AsyncEvent();
        AsyncEvent(const AsyncEvent& rhs);
        virtual ~AsyncEvent();

        uint32_t mType;

        android::sp<Data> mData;

        // Delay before the event takes place.
        // Any negative value: event takes place synchronously on the main thread.
        // Zero: event takes place on Event Generator thread, with minimum delay.
        // Positive: event takes place on Event Generator thread after stated delay in microseconds.
        int64_t mTargetTime;

        // Optional event repeat
        android::sp<RepeatData> mRepeat;
    };

    class AsyncEventGenerator : public EventMultiThread<AsyncEvent, 256, 16 >
    {
    public:
        AsyncEventGenerator(Hwch::Interface& iface);
        virtual ~AsyncEventGenerator();
        bool Add(uint32_t eventType, int32_t delayUs);
        bool Add(uint32_t eventType, android::sp<Hwch::AsyncEvent::Data> data, int32_t delayUs);
        bool Do(uint32_t eventType, android::sp<AsyncEvent::Data> data = 0);
        virtual void Do(AsyncEvent& ev);
#ifdef HWCVAL_BUILD_HWCSERVICE_API
        bool GetHwcsHandle(void);
#endif

    private:
        Hwch::Interface& mInterface;

        bool Blank(bool blank);
        bool SuspendResume(bool suspend);
        bool WidiConnect(bool connect, AsyncEvent::WidiConnectEventData* res);
        bool WidiFencePolicy(AsyncEvent::WidiFencePolicyEventData* eventData);
        bool ModeSet(AsyncEvent::ModeChangeEventData* mc);
        bool ModeClear(AsyncEvent::ModeChangeEventData* mc);
        bool SetVideoOptimizationMode(AsyncEvent::VideoOptimizationModeData* eventData);

        bool mAllowSimultaneousBlank;
        volatile int mBlankInProgress;
        bool mBlankStateRequired;

#ifdef HWCVAL_BUILD_HWCSERVICE_API
        // HWC Service Api support
        HWCSHANDLE mHwcsHandle = nullptr;
        bool WirelessDocking(bool entry);
#endif
    };

    // All kernel events will come from one thread.
    // We don't want to confuse HWC too much by doing hotplugs and unplugs at the same time.
    class KernelEventGenerator : public EventThread<AsyncEvent, 256>
    {
    public:
        KernelEventGenerator();
        virtual ~KernelEventGenerator();
        bool Add(uint32_t eventType, android::sp<AsyncEvent::Data> data, int32_t delayUs, android::sp<AsyncEvent::RepeatData> repeatData);
        bool Do(uint32_t eventType, android::sp<AsyncEvent::Data> data, android::sp<AsyncEvent::RepeatData> repeatData);

        void SetEsdConnectorId(uint32_t conn);
        void ClearContinuous();

        void GetCounts(uint32_t& hotUnplugCount, uint32_t& esdRecoveryCount);
        void ResetCounts();

    protected:
        virtual bool threadLoop();

    private:
        uint32_t mEsdConnectorId;

        uint32_t mHotUnplugCount;
        uint32_t mEsdRecoveryCount;

        volatile bool mContinueRepeat;
        volatile bool mRepeating;

        Hwcval::Watchdog mHotPlugWatchdog;

        bool SendEsdRecoveryEvent();
        bool HotPlug(bool connect, uint32_t displayTypes);
        bool ModeSet(AsyncEvent::ModeChangeEventData* mc);
        bool ModeClear(AsyncEvent::ModeChangeEventData* mc);
    };
}

#endif // __HwchAsyncEventGenerator_h__
