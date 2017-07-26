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
