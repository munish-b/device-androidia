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

#include "HwchAsyncEventGenerator.h"
#include "HwchSystem.h"
#include "HwcTestState.h"
#include "HwchInterface.h"

#ifdef HWCVAL_BUILD_HWCSERVICE_API
#include "hwcserviceapi.h"
#endif

using namespace hwcomposer;

const uint32_t Hwch::AsyncEvent::cFixedDisplay = HwcTestState::eFixed;
const uint32_t Hwch::AsyncEvent::cRemovableDisplay = HwcTestState::eRemovable;
const uint32_t Hwch::AsyncEvent::cAllDisplays = HwcTestState::eFixed | HwcTestState::eRemovable;

Hwch::AsyncEvent::AsyncEvent()
  : mType(0),
    mTargetTime(0)
{
}

Hwch::AsyncEvent::AsyncEvent(const Hwch::AsyncEvent& rhs)
  : mType(rhs.mType),
    mData(rhs.mData),
    mTargetTime(rhs.mTargetTime)
{
}

Hwch::AsyncEvent::~AsyncEvent()
{
}

android::String8 Hwch::AsyncEvent::EventName(uint32_t type)
{
    android::String8 result;

    if (type & AsyncEvent::eBlank)
    {
        result += "+Blank";
    }

    if (type & AsyncEvent::eUnblank)
    {
        result += "+Unblank";
    }

    if (type & AsyncEvent::eSuspend)
    {
        result += "+Suspend";
    }

    if (type & AsyncEvent::eResume)
    {
        result += "+Resume";
    }

    if (type & AsyncEvent::eESDRecovery)
    {
        result += "+ESDRecovery";
    }

    if (type & AsyncEvent::eHotPlug)
    {
        result += "+HotPlug";
    }

    if (type & AsyncEvent::eHotUnplug)
    {
        result += "+HotUnplug";
    }

    if (type & AsyncEvent::eModeSet)
    {
        result += "+ModeSet";
    }

    if (type & AsyncEvent::eModeClear)
    {
        result += "+ModeClear";
    }

    if (type & AsyncEvent::eSetVideoOptimizationMode)
    {
        result += "+SetVideoOptimizationMode";
    }

    if (type & AsyncEvent::eWidiConnect)
    {
        result += "+WidiConnect";
    }

    if (type & AsyncEvent::eWidiDisconnect)
    {
        result += "+WidiDisconnect";
    }

    if (type & AsyncEvent::eWidiFencePolicy)
    {
        result += "+WidiFencePolicy";
    }

    if (type & AsyncEvent::eWirelessDockingEntry)
    {
        result += "+WirelessDockingEntry";
    }

    if (type & AsyncEvent::eWirelessDockingExit)
    {
        result += "+WirelessDockingExit";
    }

    return result;
}

Hwch::AsyncEvent::HotPlugEventData::HotPlugEventData(uint32_t displayTypes)
  : mDisplayTypes(displayTypes)
{
}

Hwch::AsyncEvent::HotPlugEventData::~HotPlugEventData()
{
}


Hwch::AsyncEvent::ModeChangeEventData::~ModeChangeEventData()
{
}

// AsyncEventGenerator
Hwch::AsyncEventGenerator::AsyncEventGenerator(Hwch::Interface& iface)
  : EventMultiThread("EventGenerator"),
    mInterface(iface),
    mBlankInProgress(0)
{
    SetQueueFullError(eCheckAsyncEventsDropped);
    Hwch::System::getInstance().SetEventGenerator(this);
    mAllowSimultaneousBlank = HwcTestState::getInstance()->IsOptionEnabled(eOptSimultaneousBlank);

    HWCLOGD("Starting EventGenerator thread");
    EnsureRunning();
}

Hwch::AsyncEventGenerator::~AsyncEventGenerator()
{
#ifdef HWCVAL_BUILD_HWCSERVICE_API
    // Disconnect from the HWC Service Api
    if (mHwcsHandle)
    {
        HwcService_Disconnect(mHwcsHandle);
    }
#endif
}

bool Hwch::AsyncEventGenerator::Add(uint32_t eventType, int32_t delayUs)
{
    if (Size() + 8 < MaxSize())
    {
        return Add(eventType, 0, delayUs);
    }
    else
    {
        HWCLOGD("AsyncEventGenerator: queue full, dropped event %s", AsyncEvent::EventName(eventType).string());
        return false;
    }
}

bool Hwch::AsyncEventGenerator::Add(uint32_t eventType, android::sp<Hwch::AsyncEvent::Data> data, int32_t delayUs)
{
    if (delayUs < 0)
    {
        HWCLOGD("AsyncEventGenerator: immediate %s", AsyncEvent::EventName(eventType).string());
        return Do(eventType, data);
    }
    else
    {
        HWCLOGD("AsyncEventGenerator: queued %s at delay %fms", AsyncEvent::EventName(eventType).string(),
            double(delayUs) / 1000);

        AsyncEvent ev;
        ev.mType = eventType;
        ev.mData = data;
        ev.mTargetTime = systemTime(SYSTEM_TIME_MONOTONIC) + (int64_t(delayUs) * HWCVAL_US_TO_NS);
        Push(ev);
        return true;
    }
}


bool Hwch::AsyncEventGenerator::SuspendResume(bool suspend)
{
    if (!suspend)
    {
        // On CHT FFD, wakeup is done by issuing a timed wakeup just before we suspend.
        // Therefore we don't explicitly perform a resume.
        //
        // On BYT, we are supposed to be able to resume by writing "mem" to
        // /sys/power/state, but I found this left the system in an unstable state
        // so although the code is below I don't currently support this.
        return true;
    }

    FILE* f = fopen("/sys/class/rtc/rtc0/wakealarm", "w");
    if (f)
    {
        int64_t t = int64_t(time(0));
        fprintf(f, "%" PRIi64 "\n", (t + HWCH_SUSPEND_DURATION));
        fclose(f);
    }
    else
    {
        HWCLOGW("Failed to set wakealarm");
        return false;
    }

    f = fopen("/sys/power/state", "w");
    if (f)
    {
        HWCLOGD("Sending %s event...", suspend ? "suspend" : "resume");
        if (suspend)
        {
            HwcTestState::getInstance()->SetSuspend(true);
            fprintf(f, "mem\n");
            fclose(f);

        }
        else
        {
            fprintf(f, "on\n");
            fclose(f);
            HwcTestState::getInstance()->SetSuspend(false);
        }

        HWCLOGD("Sent %s event", suspend ? "suspend" : "resume");
        return true;
    }
    else
    {
        HWCLOGW("Failed to send %s event, can't open /sys/power/state.", suspend ? "suspend" : "resume");
    }
    return false;
}

bool Hwch::AsyncEventGenerator::Blank(bool blank)
{
    int blankReq = blank ? 1 : 0;

    if (mAllowSimultaneousBlank)
    {
        if (mInterface.GetDevice())
        {
            // Issue blank for every DRM display (not WiDi).
            for (uint32_t d=0; d<min(mInterface.NumDisplays(), 2U); ++d)
            {
                if (Hwch::System::getInstance().GetDisplay(d).IsConnected())
                {
                    if (mInterface.IsBlanked(d) != blankReq)
                    {
                        mInterface.Blank(d, blankReq);
                    }
                }
            }
        }
    }
    else if (android_atomic_swap(1, &mBlankInProgress) == 0)
    {
        mBlankStateRequired = blank;

        if (mInterface.GetDevice())
        {
            // Issue blank for every DRM display (not WiDi).
            for (uint32_t d=0; d<min(mInterface.NumDisplays(), 2U); ++d)
            {
                if (Hwch::System::getInstance().GetDisplay(d).IsConnected())
                {
                    if (mInterface.IsBlanked(d) != blankReq)
                    {
                        mInterface.Blank(d, blankReq);
                    }
                }
            }

            if (mBlankStateRequired != blank)
            {
                // Requeue the last event we threw away
                Add(mBlankStateRequired ? AsyncEvent::eBlank : AsyncEvent::eResume,
                    0, 0);
            }
        }

        mBlankInProgress = 0;
    }
    else
    {
        HWCLOGI("AsyncEventGenerator: Deferring/skipping Blank because already in progress");
        mBlankStateRequired = blank;
    }

    return true;
}

bool Hwch::AsyncEventGenerator::WidiConnect(bool connect, AsyncEvent::WidiConnectEventData* eventData)
{
    android::status_t ret = android::UNKNOWN_ERROR;

    return (ret == android::NO_ERROR);
}

bool Hwch::AsyncEventGenerator::WidiFencePolicy(AsyncEvent::WidiFencePolicyEventData* eventData)
{
    using FenceReleaseMode = Hwch::System::FenceReleaseMode;

    ALOG_ASSERT(eventData);

    FenceReleaseMode mode = static_cast<FenceReleaseMode>(eventData->mFencePolicyMode);

    HWCLOGD_COND(eLogHarness, "Harness simulating widi fence policy change to: %s",
        (mode == FenceReleaseMode::eSequential) ? "sequential" :
        (mode == FenceReleaseMode::eRandom) ? "random" :
        "retain oldest"
    );

    if (mode == FenceReleaseMode::eRetainOldest)
    {
        Hwch::System::getInstance().SetWirelessBeforeOldest(eventData->mRetainOldest);
        HWCLOGD_COND(eLogHarness, "'Retain Oldest' fence release mode selected. Retaining " \
        "oldest fence for %d frames before signalling", eventData->mRetainOldest);
    }

    Hwch::System::getInstance().SetWirelessFenceReleaseMode(mode);

    return true;
}

bool Hwch::AsyncEventGenerator::ModeSet(Hwch::AsyncEvent::ModeChangeEventData* mc)
{
    return false;
}

bool Hwch::AsyncEventGenerator::ModeClear(Hwch::AsyncEvent::ModeChangeEventData* mc)
{
    return false;
}

bool Hwch::AsyncEventGenerator::SetVideoOptimizationMode(AsyncEvent::VideoOptimizationModeData* eventData)
{
    return false;
}

#ifdef HWCVAL_BUILD_HWCSERVICE_API
bool Hwch::AsyncEventGenerator::WirelessDocking(bool entry)
{
    ALOG_ASSERT(GetHwcsHandle());

    status_t st = HwcService_Widi_SetSingleDisplay(mHwcsHandle, entry ? HWCS_TRUE : HWCS_FALSE);

    return (st == 0);
}
#endif

bool Hwch::AsyncEventGenerator::Do(uint32_t eventType, android::sp<AsyncEvent::Data> data)
{
    HWCLOGD("AsyncEventGenerator: Issuing %s", AsyncEvent::EventName(eventType).string());
    bool success = true;
    if (eventType & AsyncEvent::eBlank)
    {
        success &= Blank(true);
    }

    if (eventType & AsyncEvent::eSuspend)
    {
        success &= SuspendResume(true);
    }
    else if (eventType & AsyncEvent::eResume)
    {
        success &= SuspendResume(false);

        if (eventType & AsyncEvent::eUnblank)
        {
            // Do the unblank after the resume.
            success &= Blank(false);
        }
    }
    else if (eventType & AsyncEvent::eUnblank)
    {
        success &= Blank(false);
    }

    if (eventType & AsyncEvent::eWidiConnect)
    {
        AsyncEvent::WidiConnectEventData* res = static_cast<AsyncEvent::WidiConnectEventData*> (data.get());
        success &= WidiConnect(true, res);
    }

    if (eventType & AsyncEvent::eWidiDisconnect)
    {
        success &= WidiConnect(false, nullptr);
    }

    if (eventType & AsyncEvent::eWidiFencePolicy)
    {
        AsyncEvent::WidiFencePolicyEventData* fence_policy = static_cast<AsyncEvent::WidiFencePolicyEventData*> (data.get());
        success &= WidiFencePolicy(fence_policy);
    }

#ifdef HWCVAL_BUILD_HWCSERVICE_API
    if (eventType & AsyncEvent::eWirelessDockingEntry)
    {
        success &= WirelessDocking(true);
    }

    if (eventType & AsyncEvent::eWirelessDockingExit)
    {
        success &= WirelessDocking(false);
    }
#endif

    if (eventType & AsyncEvent::eModeSet)
    {
        // can't upcast a smart pointer
        AsyncEvent::ModeChangeEventData* mc = static_cast<AsyncEvent::ModeChangeEventData*> (data.get());
        success &= ModeSet(mc);
    }

    if (eventType & AsyncEvent::eModeClear)
    {
        // can't upcast a smart pointer
        AsyncEvent::ModeChangeEventData* mc = static_cast<AsyncEvent::ModeChangeEventData*> (data.get());
        success &= ModeClear(mc);
    }

    if (eventType & AsyncEvent::eSetVideoOptimizationMode)
    {
        // can't upcast a smart pointer
        AsyncEvent::VideoOptimizationModeData* eventData = static_cast<AsyncEvent::VideoOptimizationModeData*> (data.get());
        success &= SetVideoOptimizationMode(eventData);
    }

    HWCLOGD("AsyncEventGenerator: Issued %s, %s", AsyncEvent::EventName(eventType).string(), success ? "SUCCESS" : "FAIL");
    return success;
}

void Hwch::AsyncEventGenerator::Do(AsyncEvent& ev)
{
    int64_t t = systemTime(SYSTEM_TIME_MONOTONIC);
    int64_t delayNs = ev.mTargetTime - t;
    //HWCLOGV("%s scheduled for %f", AsyncEvent::EventName(ev.mType).string(), double(ev.mTargetTime) / HWCVAL_SEC_TO_NS);

    if (delayNs > 0)
    {
        usleep(int32_t(delayNs / HWCVAL_US_TO_NS));
    }

    if (!Do(ev.mType, ev.mData))
    {
        HWCLOGI("ASync event generation failure: %s", AsyncEvent::EventName(ev.mType).string());
    }
}

#ifdef HWCVAL_BUILD_HWCSERVICE_API
bool Hwch::AsyncEventGenerator::GetHwcsHandle()
{
    if (!mHwcsHandle)
    {
        // Attempt to connect to the new HWC Service Api
        mHwcsHandle = HwcService_Connect();

        if (!mHwcsHandle)
        {
            HWCERROR(eCheckSessionFail, "HWC Service Api could not connect to service");
            return false;
        }
    }

    return true;
}
#endif

// KernelEventGenerator
Hwch::KernelEventGenerator::KernelEventGenerator()
  : EventThread("EventGenerator"),
    mEsdConnectorId(0),
    mHotUnplugCount(0),
    mEsdRecoveryCount(0),
    mContinueRepeat(false),
    mRepeating(false),
    mHotPlugWatchdog(15 * HWCVAL_SEC_TO_NS, eCheckHotPlugTimeout, "mHotPlugWatchdog")
{
    SetQueueFullError(eCheckAsyncEventsDropped);
    Hwch::System::getInstance().SetKernelEventGenerator(this);
    HWCLOGD("Starting KernelEventGenerator thread");
    EnsureRunning();
}

Hwch::KernelEventGenerator::~KernelEventGenerator()
{
}

void Hwch::KernelEventGenerator::ClearContinuous()
{
    if (mContinueRepeat)
    {
        HWCLOGD("DISABLING continuous hot plug/ESD recovery events");

        // Wait for it to stop (up to 10ms)
        mContinueRepeat = false;
        for (uint32_t i=0; i<10; ++i)
        {
            if (!mRepeating)
            {
                break;
            }

            usleep(1000);
        }
    }
}

void Hwch::KernelEventGenerator::SetEsdConnectorId(uint32_t conn)
{
    mEsdConnectorId = conn;
}

bool Hwch::KernelEventGenerator::Add(uint32_t eventType, android::sp<AsyncEvent::Data> data, int32_t delayUs, android::sp<AsyncEvent::RepeatData> repeatData)
{
    if (delayUs < 0)
    {
        HWCLOGD("KernelEventGenerator: immediate %s", AsyncEvent::EventName(eventType).string());
        return Do(eventType, data, repeatData);
    }
    else
    {
        if (Size() + 2 < MaxSize())
        {
            HWCLOGD("KernelEventGenerator: queued %s at delay %fms", AsyncEvent::EventName(eventType).string(),
                double(delayUs) / 1000);

            if (repeatData.get())
            {
                mContinueRepeat = true;
                mRepeating = true;
            }

            AsyncEvent ev;
            ev.mType = eventType;
            ev.mData = data;
            ev.mRepeat = repeatData;
            ev.mTargetTime = systemTime(SYSTEM_TIME_MONOTONIC) + (int64_t(delayUs) * HWCVAL_US_TO_NS);
            Push(ev);
            return true;
        }
        else
        {
            HWCLOGD("KernelEventGenerator: dropped %s", AsyncEvent::EventName(eventType).string());
            return false;
        }
    }
}

bool Hwch::KernelEventGenerator::SendEsdRecoveryEvent()
{
    uint32_t connectorId = mEsdConnectorId;

    if (connectorId == 0)
    {
        connectorId = HwcTestState::getInstance()->GetDisplayProperty(0, HwcTestState::ePropConnectorId);
    }

    if (connectorId > 0)
    {
        HWCLOGD("Sending ESD recovery event to connector %d...", connectorId);
        HwcTestState::getInstance()->MarkEsdRecoveryStart(connectorId);

        FILE* f = fopen("/sys/kernel/debug/dri/0/i915_connector_reset", "w");
        if (f)
        {
            fprintf(f, "%d\n", connectorId);
            fclose(f);
            HWCLOGD("Sent ESD recovery event to connector %d", connectorId);
            ++mEsdRecoveryCount;
            return true;
        }
        else
        {
            HWCERROR(eCheckTestFail, "Failed to issue ESD recovery event - can't open /sys/kernel/debug/dri/0/i915_connector_reset");
        }
    }
    else
    {
        HWCERROR(eCheckTestFail, "Failed to issue ESD recovery event - no valid connector id for panel");
    }
    return false;
}

bool Hwch::KernelEventGenerator::HotPlug(bool connect, uint32_t displayTypes)
{
    HwcTestState* state = HwcTestState::getInstance();

    if (!Hwch::System::getInstance().IsHDMIToBeTested())
    {
        return false;
    }

    HWCLOGD_COND(eLogHotPlug, "Harness simulating hot %splugging to %s", (connect ? "" : "un"), HwcTestState::DisplayTypeStr(displayTypes));

    mHotPlugWatchdog.Start();
    bool canHotPlug = state->SimulateHotPlug(connect, displayTypes);
    mHotPlugWatchdog.Stop();

    if (!canHotPlug)
    {
        HWCLOGI("Hot plug/unplug not available - no suitable display.");
    }

    return canHotPlug;
}

bool Hwch::KernelEventGenerator::Do(uint32_t eventType, android::sp<AsyncEvent::Data> data, android::sp<AsyncEvent::RepeatData> repeatData)
{
    bool success = true;
    uint32_t rept=repeatData.get() ? repeatData->mBurstLength : 1;

    for (uint32_t i=0; i<rept; ++i)
    {
        HWCLOGD("KernelEventGenerator: Issuing %s", AsyncEvent::EventName(eventType).string());
        if (eventType & AsyncEvent::eESDRecovery)
        {
            success &= SendEsdRecoveryEvent();
        }

        if (eventType & AsyncEvent::eHotUnplug)
        {
            AsyncEvent::HotPlugEventData* hpData = (AsyncEvent::HotPlugEventData*) data.get();
            uint32_t displayTypes = hpData ? hpData->mDisplayTypes : AsyncEvent::cAllDisplays;

            bool st = HotPlug(false, displayTypes);

            if (st)
            {
                ++mHotUnplugCount;
            }

            success &= st;
        }

        if (eventType & AsyncEvent::eHotPlug)
        {
            AsyncEvent::HotPlugEventData* hpData = (AsyncEvent::HotPlugEventData*) data.get();
            uint32_t displayTypes = hpData ? hpData->mDisplayTypes : AsyncEvent::cAllDisplays;
            success &= HotPlug(true, displayTypes);
        }

        HWCLOGD("KernelEventGenerator: Issued %s, %s", AsyncEvent::EventName(eventType).string(), success ? "SUCCESS" : "FAIL");

        if (!mContinueRepeat)
        {
            break;
        }
    }

    if (mContinueRepeat && repeatData.get())
    {
        // Submit next burst back to the queue
        Add(eventType, data, repeatData->mDelayUs, repeatData);
    }
    else
    {
        mRepeating = false;
    }

    return success;
}

bool Hwch::KernelEventGenerator::threadLoop()
{
    // Pull each event from the queue, wait for the requested delay if any, and
    // action the event.

    while (true)
    {
        AsyncEvent ev;

        ReadWait(ev);

        int64_t t = systemTime(SYSTEM_TIME_MONOTONIC);
        int64_t delayNs = ev.mTargetTime - t;
        if (delayNs > 0)
        {
            usleep(int32_t(delayNs / HWCVAL_US_TO_NS));
        }

        if (!Do(ev.mType, ev.mData, ev.mRepeat))
        {
            HWCLOGI("Kernel event generation failure: %s", AsyncEvent::EventName(ev.mType).string());
        }
    }

    return true;
}

void Hwch::KernelEventGenerator::GetCounts(uint32_t& hotUnplugCount, uint32_t& esdRecoveryCount)
{
    hotUnplugCount = mHotUnplugCount;
    esdRecoveryCount = mEsdRecoveryCount;
}

void Hwch::KernelEventGenerator::ResetCounts()
{
    mHotUnplugCount = 0;
    mEsdRecoveryCount = 0;
}
