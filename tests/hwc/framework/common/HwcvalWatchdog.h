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

File Name:      HwcvalWatchdog.h

Description:    Class definition for a Watchdog timer.

Environment:

Notes:

****************************************************************************/
#ifndef __HwcvalWatchdog_h__
#define __HwcvalWatchdog_h__

#include <stdint.h>
#include "HwcTestState.h"
#include <utils/String8.h>

namespace Hwcval
{

    class Watchdog : public android::RefBase
    {
    public:
        Watchdog(uint64_t ns, HwcTestCheckType check, const char* str="Watchdog timer");
        Watchdog(Watchdog& rhs);

        void SetMessage(const android::String8& str);
        void Start();
        void StartIfNotRunning();
        void Stop();
        int64_t GetStartTime();

    private:
        static void TimerHandler(sigval_t value);
        void TimerHandler();

        uint64_t mTimeoutNs;
        bool mHaveTimer;
        bool mRunning;
        timer_t mDelayTimer;
        int64_t mStartTime;

        HwcTestCheckType mCheck;
        android::String8 mMessage;
    };

    inline int64_t Watchdog::GetStartTime()
    {
        return mStartTime;
    }


} // namespace Hwcval

#endif // __HwcvalWatchdog_h__
