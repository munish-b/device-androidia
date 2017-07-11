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

File Name:      HwchVSync.h

Description:    Class definition for HWC harness VShync interception class

Environment:

Notes:

****************************************************************************/
#ifndef __HwcVSync_h__
#define __HwcVSync_h__

#include <utils/Thread.h>
#include "HwcvalDebug.h"


namespace Hwch
{

    class VSync
    {
        public:
            VSync();
            virtual ~VSync();

            // Set the delay in microseconds between VSync and condition being signalled.
            void SetVSyncDelay(uint32_t delayus);

            // Set the timeout in microseconds for when VSync does not come.
            void SetTimeout(uint32_t timeoutus);

            // Set the period we will use when VSyncs don't come within the timeout.
            void SetRequestedVSyncPeriod(uint32_t periodus);

            // Stop handling VSyncs
            void Stop();

            void WaitForOffsetVSync();

            // VSync callback
            virtual void Signal(uint32_t disp);

            bool IsActive();

        private:
            // Delay in nanoseconds between VSync and condition being signalled.
            uint32_t mDelayns;

            // Timeout in ns for when VSyncs don't occur.
            uint32_t mTimeoutns;

            // Expected time between VSyncs. We will simulate this if the real ones don't occur within the timeout period.
            uint32_t mRequestedVSyncPeriodns;

            timer_t mDelayTimer;

            Hwcval::Condition mCondition;
            Hwcval::Mutex mMutex;

            bool mActive;       // Vsyncs are happening
            bool mHaveTimer;    // Timer has been set up

            volatile int64_t mOffsetVSyncTime;
            int64_t mLastConsumedOffsetVSyncTime;

            static void TimerHandler(sigval_t value);
            void OffsetVSync();
            void DestroyTimer();
    };

    inline bool VSync::IsActive()
    {
        return mActive;
    }

}

#endif // __HwchVSync_h__
