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
* File Name:            HwchTimelineThread.h
*
* Description:          Timeline thread class definition
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#ifndef __HwchTimelineThread_h__
#define __HwchTimelineThread_h__

#include "utils/Thread.h"
#include "HwchDefs.h"


namespace Hwch
{
    class Interface;

    class TimelineThread : public android::Thread
    {
    public:
        TimelineThread();
        virtual ~TimelineThread();

        int Get();
        uint32_t GetTimelineTime();

    private:
        //Thread functions
        virtual bool threadLoop();
        virtual android::status_t readyToRun();

        // Private data
        volatile int mTimeline;              // Timeline handle
        volatile uint32_t mTimelineTime;     // Current timeline time
        int64_t mLastRealTime;
    };
}

#endif // __HwchTimelineThread_h__
