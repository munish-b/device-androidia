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
* File Name:            HwcTestTimeline.h
*
* Description:          Timeline thread class definition
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#ifndef __HwcTestTimeline_h__
#define __HwcTestTimeline_h__

#include "utils/String8.h"
#include "utils/KeyedVector.h"
#include "HwcTestDefs.h"

struct HwcTestTimelineData
{
    int outFence;
    uint32_t timelineSeq;
};

class HwcTestTimeline
{
public:
    HwcTestTimeline(uint32_t id);
    virtual ~HwcTestTimeline();

    // Create a fence on the next timeline sequence
    // Add it to the map, so we know which input fence it represents
    int CreateShimFence(int inFence);

    // Close the input fence.
    // Find it in the map. If it's there, prepare to add one to the shim timeline
    // (but don't do it now as we want it to be atomic).
    // Remove the input fence from the map.
    void CloseFence(int inFence);

    // Signal all shim fences up to the last one where CloseFence was called on the input fence.
    void SignalShimFence();

private:
    // Private data
    volatile int mTimeline;             // Timeline handle
    volatile uint32_t mTimelineSeq;     // Timeline seq for fences we create
    uint32_t mCurrentSeq;               // Last signalled seq
    uint32_t mFencesClosed;             // Count of in fences closed
    android::String8 mName;
};

#endif // __HwcTestTimeline_h__
