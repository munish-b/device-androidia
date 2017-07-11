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
 * @file    ReplayLayer.h
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    3rd July 2014
 * @brief   Layer sub-class for the replay functionality.
 *
 * @details This file sub-classes HwchLayer to allow the replay mechanism to
 * track the buffer handles that are associated with a layer.
 *
 *****************************************************************************/

#ifndef __HwchReplayLayer_h__
#define __HwchReplayLayer_h__

#include <utils/RefBase.h>
#include <utils/SortedVector.h>
#include <utils/Errors.h>

#include "HwchCoord.h"
#include "HwchLayer.h"
#include "HwchSystem.h"

namespace Hwch
{

    class ReplayLayer :
        public Layer,
        public android::RefBase
    {
        private:

            /**
             * A SortedVector of buffer handles that are associated with this
             * layer. Provides lookup and insertion in O(log N) time.
             */
            android::SortedVector<uint32_t> mKnownBufs;

            /**
             * Cache the last handle seen by this layer. This is necessary for
             * the buffer rotation code i.e. layers that are triple-buffered
             * have three buffer handls associated with them.
             */
            uint32_t mLastHandle = 0;

        public:

            /**
             * @name  ReplayLayer
             * @brief Base class constructor.
             *
             * @param name Name of the layer e.g. StatusBar
             * @param width Layer width (in pixels)
             * @param height Layer height (in pixels)
             * @param format Defines the clour space format
             *
             * @details Calls the parent class constructor.
             */
            ReplayLayer(const char* name, Coord<int32_t> width,
                            Coord<int32_t> height,
                            uint32_t format = HAL_PIXEL_FORMAT_RGBA_8888,
                            uint32_t bufs = 1) :
                            Layer(name, width, height, format, bufs)
                            {};

            /** Copy constructor (required for Dup). */
            ReplayLayer(const ReplayLayer& rhs) :
                Layer(rhs),
                android::RefBase(),
                mKnownBufs(rhs.mKnownBufs) {};

            /** Associates a handle with the layer and returns its index. */
            size_t AddKnownBuffer(uint32_t handle)
            {
                return mKnownBufs.add(handle);
            }

            /** Tests whether a handle is associated with the layer. */
            bool IsKnownBuffer(uint32_t handle) const
            {
                return (mKnownBufs.indexOf(handle) != android::NAME_NOT_FOUND);
            }

            /** Returns the index of a handle in the vector (if it exists). */
            size_t GetKnownBufferIndex(uint32_t handle) const
            {
                return mKnownBufs.indexOf(handle);
            }

            /** Returns the number of handles that are known to this layer */
            size_t GetNumHandles() const
            {
                return mKnownBufs.size();
            }

            /** Sets the last handle seen on this layer. */
            void SetLastHandle(uint32_t handle)
            {
                mLastHandle = handle;
            }

            /** Returns the last handle seen on this layer. */
            uint32_t GetLastHandle() const
            {
                return mLastHandle;
            }

            /**
             * Returns whether the layer fills the screen (e.g. Wallpaper).
             *
             * Note: this function uses the coordinates of the layer's logical
             * display frame to determine whether or not it is full screen. This
             * is fine in the Replay tool, but may be invalid in other contexts.
             */
            bool IsFullScreen(uint32_t display)
            {
                LogDisplayRect& ldf = mLogicalDisplayFrame;
                Display& system_display = mSystem.GetDisplay(display);

                return (((ldf.bottom.mValue - ldf.top.mValue) >=
                            static_cast<int32_t>(system_display.GetHeight())) &&
                        ((ldf.right.mValue - ldf.left.mValue) >=
                             static_cast<int32_t>(system_display.GetWidth())));
            }

            /** Overrides Dup so that we can duplicate layers as required. */
            ReplayLayer* Dup() override
            {
                return new ReplayLayer(*this);
            }
    };
}

#endif // __HwchReplayLayer_h__
