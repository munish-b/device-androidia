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
* File Name:            HwchWatchdogThread.cpp
*
* Description:          Watchdog thread class implementation
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#include "HwchWatchdogThread.h"
#include "HwcTestState.h"
#include "HwcTestUtil.h"
#include "HwcvalThreadTable.h"
#include "HwcHarness.h"
#include "HwchDefs.h"
#include "HwchFrame.h"

Hwch::WatchdogThread::WatchdogThread(HwcTestRunner* runner)
  : mStartTime(0),
    mRunner(runner)
{
    Hwcval::ValCallbacks::Set(this);
    run("HwchWatchdogThread", android::PRIORITY_NORMAL);
}

Hwch::WatchdogThread::~WatchdogThread()
{
    HWCLOGI("WatchdogThread::~WatchdogThread()");
    requestExitAndWait();
}

android::status_t Hwch::WatchdogThread::readyToRun()
{
    return android::NO_ERROR;
}

void Hwch::WatchdogThread::Set(uint32_t minMinutes, float minFps)
{
    mMinNs = ((int64_t)minMinutes) * 60 * HWCVAL_SEC_TO_NS;
    mMinFps = minFps;
}

void Hwch::WatchdogThread::Start()
{
    mStartTime = systemTime(SYSTEM_TIME_MONOTONIC);
}

void Hwch::WatchdogThread::Stop()
{
    mStartTime = 0;
}

void Hwch::WatchdogThread::Exit()
{
    ALOGE("Unrecoverable error detected. Aborting HWC harness...");
    mRunner->Lock();
    ALOGD("Runner lock obtained.");
    Hwcval::ReportThreadStates();
    mRunner->LogTestResult();
    mRunner->LogSummary();
    mRunner->WriteCsvFile();
    mRunner->CombineFiles(0);
    Hwch::System::QuickExit();
}

bool Hwch::WatchdogThread::threadLoop()
{
    uint32_t lastFrameCount = 0;
    uint32_t noChangeCount = 0;

    while (!exitPending())
    {
        // Check every minute
        sleep(60);

        if (mStartTime != 0)
        {
            uint64_t currentTime = systemTime(SYSTEM_TIME_MONOTONIC);
            uint64_t runTime = currentTime - mStartTime;
            HwcTestState* state = HwcTestState::getInstance();

            if (runTime > mMinNs)
            {
                state->ReportFrameCounts(false);
                uint32_t tooSlowDisplaysCount = 0;
                int fastestDisplay = -1;
                float fastestFps = 0;
                uint32_t fastestFrames = 0;

                for (uint32_t d=0; d<HWCVAL_MAX_CRTCS; ++d)
                {
                    uint32_t frames = HwcGetTestResult()->mPerDisplay[d].mFrameCount;
                    float fps = (float(frames) * HWCVAL_SEC_TO_NS) / runTime;

                    if (fps < mMinFps)
                    {
                        ++tooSlowDisplaysCount;

                        if (fps > fastestFps)
                        {
                            fastestFps = fps;
                            fastestDisplay = d;
                            fastestFrames = frames;
                        }
                    }
                }

                if (tooSlowDisplaysCount == HWCVAL_MAX_CRTCS)
                {
                    HWCERROR(eCheckTooSlow, "Test has achieved %d frames on D%d in %d seconds (%f fps), below minimum frame rate of %3.1f fps",
                        fastestFrames, fastestDisplay, int(runTime / HWCVAL_SEC_TO_NS), double(fastestFps), double(mMinFps));
                    Exit();
                }
            }

            uint32_t framesNow = Hwch::Frame::GetFrameCount();

            if (framesNow == lastFrameCount)
            {
                ++noChangeCount;

                if (noChangeCount >= HWCH_WATCHDOG_INACTIVITY_MINUTES)
                {
                    float fps = (float(framesNow) * HWCVAL_SEC_TO_NS) / runTime;

                    HWCERROR(eCheckTooSlow, "Test has achieved %d frames in %d seconds (%f fps) and no frames for last %d minutes.",
                        framesNow, int(runTime / HWCVAL_SEC_TO_NS), double(fps), HWCH_WATCHDOG_INACTIVITY_MINUTES);
                    Exit();
                }
            }
            else
            {
                lastFrameCount = framesNow;
                noChangeCount = 0;
            }



            // Check for iVP lockups
            uint64_t ivpStartTime = HwcTestState::getInstance()->GetIVPStartTime();
            if (ivpStartTime)
            {
                if (currentTime >= ivpStartTime + (60 * HWCVAL_SEC_TO_NS))
                {
                    HWCERROR(eCheckIvpLockUp, "Watchdog has detected iVP lock-up (iVP_exec has taken > 1 minute to return) - exiting!");
                    Exit();
                }
            }
        }
    }

    HWCLOGI("WatchdogThread::threadLoop exiting");
    return false;
}
