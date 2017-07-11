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
* File Name:            HwcTestTimeline.cpp
*
* Description:          Timeline thread class implementation
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#include "HwcTestTimeline.h"
#include "HwcTestState.h"
#include "HwcTestUtil.h"

#include <unistd.h>


HwcTestTimeline::HwcTestTimeline(uint32_t id)
  : mTimelineSeq(0),
    mCurrentSeq(0),
    mFencesClosed(0)
{
    mTimeline = sw_sync_timeline_create();
    mName = android::String8::format("HwcTestTimeline %d", id);
}

HwcTestTimeline::~HwcTestTimeline()
{
    HWCLOGI("HwcTestTimeline::~HwcTestTimeline()");
    CloseFence(mTimeline);
}


// Create a fence on the next timeline sequence
// Add it to the map, so we know which input fence it represents
int HwcTestTimeline::CreateShimFence(int inFence)
{
    ++mTimelineSeq;

    int outFence = sw_sync_fence_create(mTimeline, mName.string(), mTimelineSeq);
    HWCLOGD_COND(eLogEventQueue, "%s::CreateShimFence inFence: %d Timeline: %d outFence: %d", mName.string(), inFence, mTimelineSeq, outFence);

    return outFence;
}

// Close the input fence.
// Keep a count so we know how much to advance the timeline by.
void HwcTestTimeline::CloseFence(int inFence)
{
    HWCLOGV_COND(eLogEventQueue, "%s: Closed in fence %d",  mName.string(), inFence);
    ++mFencesClosed;

    close(inFence);
}

// Find the shim fence mapping to the input fence and signal it.
// Also close the input fence which we assume is signalled.
void HwcTestTimeline::SignalShimFence()
{
    int timelineIncrement = mFencesClosed - mCurrentSeq;

    if (timelineIncrement > 0)
    {
        HWCLOGD_COND(eLogEventQueue, "%s: Increment %d", mName.string(), timelineIncrement);
        mCurrentSeq = mFencesClosed;
        sw_sync_timeline_inc(mTimeline, timelineIncrement);
    }
    else
    {
        HWCLOGD_COND(eLogEventQueue, "%s: NO INCREMENT!");
    }
}


