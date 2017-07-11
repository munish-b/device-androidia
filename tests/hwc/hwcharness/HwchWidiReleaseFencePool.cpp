/****************************************************************************
 *
 * Copyright (c) Intel Corporation (2015).
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
 * @file    WidieleaseFencePool.cpp
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    27th January 2015 (split from Widi)
 * @brief   Manages a pool of release fences for the Widi.
 *
 * @details This class implements a pool of release fences that can be signalled
 *          in-order or randomly. This is designed to recreate the behaviour of
 *          the Widi stack and to stress the internal buffering of the HWC. The
 *          pool size is defined by mPoolSize. Note: each fence is created on its
 *          own timeline. Timelines are recycled when fences are signalled.
 *
 *****************************************************************************/

#include "HwcTestState.h"
#include "HwchSystem.h"

#include "HwchWidiReleaseFencePool.h"

// Default constructor
Hwch::WidiReleaseFencePool::WidiReleaseFencePool()
{
    Hwch::System& system = Hwch::System::getInstance();

    // Update the pool size and the number of frames required before signalling
    // the oldest frame.
    mPoolSize = system.GetWirelessFencePoolSize();
    mBeforeOldest = system.GetWirelessBeforeOldest();

    // Initialise the timelines
    mTimelinePool = new timeline_pool_node_t[mPoolSize];
    if (!mTimelinePool)
    {
        HWCLOGD_COND(eLogWidi, "%s: error allocating timelines!", __func__);
        return;
    }

    for (uint32_t i = 0; i<mPoolSize; ++i)
    {
        // Create the timelines
        mTimelinePool[i].timeline = sw_sync_timeline_create();
    }
}

Hwch::WidiReleaseFencePool::~WidiReleaseFencePool()
{
    if (mTimelinePool)
    {
        delete [] mTimelinePool;
    }
}

// Allocates a release fence - returns -1 if no fences are available
int Hwch::WidiReleaseFencePool::GetReleaseFence()
{
    for (uint32_t i = 0; i<mPoolSize; ++i)
    {
        if (mTimelinePool[i].pendingFence == -1)
        {
            // Try to allocate a fence
            mTimelinePool[i].pendingFence = sw_sync_fence_create(mTimelinePool[i].timeline,
                "ReleaseFencePool", mTimelinePool[i].timelineTime + 1);
            if (mTimelinePool[i].pendingFence <= 0)
            {
                HWCLOGD_COND(eLogWidi, "%s: error allocating fence!", __func__);
                return -1;
            }

            // Cache this index in 'age' order
            mIndexAges.push_back(i);
            HWCLOGV_COND(eLogWidi, "Added %d to mIndexAges", i);

            return mTimelinePool[i].pendingFence;
        }
    }

    // The pool is full
    return -1;
}

// Signal a specific fence (index)
bool Hwch::WidiReleaseFencePool::SignalIndex(uint32_t index)
{
    if ((index >= mPoolSize) || (mTimelinePool[index].pendingFence == -1))
    {
        return false;
    }

    sw_sync_timeline_inc(mTimelinePool[index].timeline, 1);
    mTimelinePool[index].timelineTime++;
    mTimelinePool[index].pendingFence = -1;
    mLastSignalledIndex = index;

    // Remove this index from the 'age' array
    HWCLOGV_COND(eLogWidi, "Removing %d from mIndexAges", index);

    for(uint32_t i=0; i<mIndexAges.size(); ++i)
    {
        if (index == mIndexAges[i])
        {
            mIndexAges.removeAt(i);
            break;
        }
    }

    return true;
}

// Signal a random fence
bool Hwch::WidiReleaseFencePool::SignalRandomIndex()
{
    uint32_t bucket_size = RAND_MAX / mPoolSize;
    uint32_t index;
    do // protects against rand() returning RAND_MAX
    {
        index = rand() / bucket_size;
    } while (index == mPoolSize);

    return SignalIndex(index);
}

// Signal the next index in age order
bool Hwch::WidiReleaseFencePool::SignalSequentialIndex()
{
    return SignalIndex(mIndexAges[0]);
}

// Signal the oldest frame
bool Hwch::WidiReleaseFencePool::SignalOldestIndex()
{
    if (mIndexAges.size() < mPoolSize)
    {
        return false;
    }

    if (mBeforeOldestCount == mBeforeOldest)
    {
        mBeforeOldestCount = 0;
        return SignalIndex(mIndexAges[0]); // Signal oldest
    }
    else
    {
        mBeforeOldestCount++;
        return SignalIndex(mIndexAges[1]); // Signal second oldest
    }
}
