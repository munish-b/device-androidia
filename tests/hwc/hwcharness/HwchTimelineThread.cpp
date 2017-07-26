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

#include "HwchTimelineThread.h"
#include "HwcTestState.h"
#include "HwcTestUtil.h"
#if ANDROID_VERSION < 430
#include <sync/sync.h>
#else
#include SW_SYNC_H_PATH
#endif

#include <unistd.h>


Hwch::TimelineThread::TimelineThread()
  : mTimelineTime(0)
{
    mTimeline = sw_sync_timeline_create();
    run("HwchTimelineThread", android::PRIORITY_NORMAL);
    mLastRealTime = systemTime(SYSTEM_TIME_MONOTONIC);
}

Hwch::TimelineThread::~TimelineThread()
{
    HWCLOGI("TimelineThread::~TimelineThread()");
    requestExitAndWait();
    HWCLOGI("TimelineThread::~TimelineThread terminated");
    CloseFence(mTimeline);
    HWCLOGI("TimelineThread::~TimelineThread exiting");
}

android::status_t Hwch::TimelineThread::readyToRun()
{
    return android::NO_ERROR;
}


bool Hwch::TimelineThread::threadLoop()
{
    while (!exitPending())
    {
        int64_t targetRealTime = mLastRealTime + NS_PER_TIMELINE_TICK;
        int64_t nsToWait = targetRealTime - systemTime(SYSTEM_TIME_MONOTONIC);

        if (nsToWait > 0)
        {
            timespec tsrq;;
            tsrq.tv_sec = 0;
            tsrq.tv_nsec = nsToWait;
            timespec tsrm;
            nanosleep(&tsrq, &tsrm);
        }

        sw_sync_timeline_inc(mTimeline, 1);

        mTimelineTime += 1;
        mLastRealTime = targetRealTime;
        HWCLOGD_COND(eLogTimeline, "Hwch::TimelineThread tick mTimelineTime = %d", mTimelineTime);
    }

    HWCLOGI("TimelineThread::threadLoop exiting");
    return false;
}

uint32_t Hwch::TimelineThread::GetTimelineTime()
{
    return mTimelineTime;
}

int Hwch::TimelineThread::Get()
{
    return mTimeline;
}
