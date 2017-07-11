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
* File Name:            HwchTimelineThread.cpp
*
* Description:          Timeline thread class implementation
*
* Environment:
*
* Notes:
*
*****************************************************************************/
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
