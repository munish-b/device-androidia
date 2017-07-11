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
 * @file    WidiReleaseFencePool.h
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    27th January 2015
 * @brief   Manages a pool of release fences for the Widi.
 *
 * @details This class implements a pool of release fences that can be signalled
 *          in-order or randomly. This is designed to recreate the behaviour of
 *          the Widi stack and to stress the internal buffering of the HWC. The
 *          pool size is defined by mPoolSize. Note: each fence is created on its
 *          own timeline. Timelines are recycled when fences are signalled.
 *
 *****************************************************************************/

#ifndef __HwchWidiReleaseFencePool_h__
#define __HwchWidiReleaseFencePool_h__

#include <stdint.h>

#include <utils/Vector.h>

#if ANDROID_VERSION < 430
#include <sync/sync.h>
#else
#include SW_SYNC_H_PATH
#endif

namespace Hwch
{
    class WidiReleaseFencePool
    {
        private:

            // Size of the fence pool
            uint32_t mPoolSize = 4;

            // Number of fences to signal before the oldest
            uint32_t mBeforeOldest = 3;

            // Retain a count of the number of fences signalled before we need
            // to signal the oldest
            uint32_t mBeforeOldestCount = 0;

            // Create a pool of timelines
            using timeline_pool_node_t = struct
            {
                volatile int32_t timeline = 0;
                int32_t timelineTime = 0;
                int32_t pendingFence = -1;
            };
            timeline_pool_node_t *mTimelinePool = NULL;

            // Store the timeline fence indexes in order of age
            android::Vector<uint32_t> mIndexAges;

            // Record the index of the fence that was signalled last
            int32_t mLastSignalledIndex = -1;

        public:

            // Class design - Big 5 plus destructor
            WidiReleaseFencePool();
            ~WidiReleaseFencePool();
            WidiReleaseFencePool(const WidiReleaseFencePool& rhs) = delete;
            WidiReleaseFencePool(WidiReleaseFencePool&& rhs) = delete;
            WidiReleaseFencePool& operator=(const WidiReleaseFencePool& rhs) = delete;
            WidiReleaseFencePool& operator=(WidiReleaseFencePool&& rhs) = delete;

            // Allocates a release fence - returns -1 if no fences are available
            int GetReleaseFence();

            // Signal a specific index
            bool SignalIndex(uint32_t index);

            // Signal a random index
            bool SignalRandomIndex();

            // Signal next index
            bool SignalSequentialIndex();

            // Signal the next 'mBeforeOldest' fences before signalling the oldest
            bool SignalOldestIndex();

            // Accessors
            int32_t GetLastSignalledIndex()
            {
                return mLastSignalledIndex;
            }

            int32_t GetPoolSize()
            {
                return mPoolSize;
            }
    };
}

#endif // __HwchWidiReleaseFencePool_h__
