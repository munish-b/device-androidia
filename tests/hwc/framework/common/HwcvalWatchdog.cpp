/****************************************************************************

Copyright (c) Intel Corporation (2014).

DISCLAIMER OF WARRANTY
NEITHER INTEL NOR ITS SUPPLIERS MAKE ANY REPRESENTATION OR WARRANTY OR
CONDITION OF ANY KIND WHETHER EXPRESS OR IMPLIED (EITHER IN FACT OR BY
OPERATION OF LAW) WITH RESPECT TO THE SOURCE CODE.  INTEL AND ITS SUPPLIERS
EXPRESSLY DISCLAIM ALL WARRANTIES OR CONDITIONS OF MERCHANTABILITY OR
FITNESS FOR A PARTICULAR PURPOSE.  INTEL AND ITS SUPPLIERS DO NOT WARRANT
THAT THE SOURCE CODE IS ERROR-FREE OR THAT OPERATION OF THE SOURCE CODE WILL
BE SECURE OR UNINTERRUPTED AND HEREBY DISCLAIM ANY AND ALL LIABILITY ON
ACCOUNT THEREOF.  THERE IS ALSO NO IMPLIED WARRANTY OF NON-INFRINGEMENT.
SOURCE CODE IS LICENSED TO LICENSEE ON AN "AS IS" BASIS AND NEITHER INTEL
NOR ITS SUPPLIERS WILL PROVIDE ANY SUPPORT, ASSISTANCE, INSTALLATION,
TRAINING OR OTHER SERVICES.  INTEL AND ITS SUPPLIERS WILL NOT PROVIDE ANY
UPDATES, ENHANCEMENTS OR EXTENSIONS.

File Name:      HwcvalWatchdog.cpp

Description:    Implmentation of Watchdog timer class.

Environment:

Notes:

****************************************************************************/

#include "HwcvalWatchdog.h"

Hwcval::Watchdog::Watchdog(uint64_t ns, HwcTestCheckType check, const char* str)
  : mTimeoutNs(ns),
    mHaveTimer(false),
    mRunning(false),
    mStartTime(0),
    mCheck(check),
    mMessage(str)
{
}

// Copy constructor
// Only copy state, ie. start time, actual timer will not be running in the copy.
Hwcval::Watchdog::Watchdog(Hwcval::Watchdog& rhs)
  : android::RefBase(),
    mTimeoutNs(rhs.mTimeoutNs),
    mHaveTimer(false),
    mRunning(false),
    mStartTime(rhs.mStartTime),
    mCheck(rhs.mCheck)
{
}

void Hwcval::Watchdog::SetMessage(const android::String8& str)
{
    mMessage = str;
}

void Hwcval::Watchdog::Start()
{
    Stop();

    mStartTime = systemTime(SYSTEM_TIME_MONOTONIC);

    if (!mHaveTimer)
    {
        struct sigevent timerEvent;
        memset(&timerEvent, 0, sizeof(timerEvent));
        timerEvent.sigev_notify = SIGEV_THREAD;
        timerEvent.sigev_notify_function = TimerHandler;
        timerEvent.sigev_value.sival_ptr = this;

        if ( 0 != timer_create(CLOCK_MONOTONIC, &timerEvent, &mDelayTimer) )
        {
            HWCLOGW("Watchdog: Failed to create timer for %s", mMessage.string());
        }
        else
        {
            mHaveTimer = true;
            mRunning = true;
            HWCCHECK(mCheck);
        }
    }

    // Reset the timer
    if ( mHaveTimer )
    {
        struct itimerspec timerSpec;
        timerSpec.it_value.tv_sec     = mTimeoutNs / HWCVAL_SEC_TO_NS;
        timerSpec.it_value.tv_nsec    = mTimeoutNs % HWCVAL_SEC_TO_NS;
        timerSpec.it_interval.tv_sec  = 0; // This is a one-hit timer so no interval
        timerSpec.it_interval.tv_nsec = 0;

        if ( 0 != timer_settime(mDelayTimer, 0, &timerSpec, NULL) )
        {
            ALOGE("Watchdog: Failed to set timer for %s", mMessage.string());
            Stop();
        }
    }
}

void Hwcval::Watchdog::StartIfNotRunning()
{
    if (!mRunning)
    {
        Start();
    }
}

void Hwcval::Watchdog::TimerHandler(sigval_t value)
{
    ALOG_ASSERT( value.sival_ptr );
    static_cast<Hwcval::Watchdog*>(value.sival_ptr)->TimerHandler();
}

void Hwcval::Watchdog::TimerHandler()
{
    mRunning = false;
    HWCERROR(mCheck, "%s timed out after %fms. Start time %f",
        mMessage.string(), (double(mTimeoutNs) / HWCVAL_MS_TO_NS), double(mStartTime) / HWCVAL_SEC_TO_NS);
}

void Hwcval::Watchdog::Stop()
{
    if ( mHaveTimer )
    {
        HWCLOGV_COND(eLogEventHandler, "%s: Cancelled after %fms", mMessage.string(),
            double(systemTime(SYSTEM_TIME_MONOTONIC) - mStartTime) / HWCVAL_MS_TO_NS);
        timer_delete(mDelayTimer);
        mHaveTimer = false;
        mRunning = false;
    }
}

