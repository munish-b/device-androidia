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
