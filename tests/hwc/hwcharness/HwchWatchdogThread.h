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
* File Name:            HwchWatchdogThread.h
*
* Description:          Watchdog thread class definition
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#ifndef __HwchWatchdogThread_h__
#define __HwchWatchdogThread_h__

#include "utils/Thread.h"
#include "HwchDefs.h"
#include "HwcTestConfig.h"

class HwcTestRunner;

namespace Hwch
{
    class WatchdogThread : public android::Thread, public Hwcval::ValCallbacks
    {
    public:
        WatchdogThread(HwcTestRunner* runner);
        virtual ~WatchdogThread();

        void Set(uint32_t minMinutes, float minFps);
        void Start();
        void Stop();
        void Exit();

    private:
        //Thread functions
        virtual bool threadLoop();
        virtual android::status_t readyToRun();

        // Private data
        // Minimum run time in ns before checks start
        uint64_t mMinNs;

        // Minimum frame rate in fps to be achieved after the minimum test run time has expired
        float mMinFps;

        // Time the test started
        volatile int64_t mStartTime;

        // The test runner
        HwcTestRunner* mRunner;
    };
}

#endif // __HwchWatchdogThread_h__
