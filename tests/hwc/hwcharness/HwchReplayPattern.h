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
 * @file    ReplayPattern.h
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    8th July 2014
 * @brief   Pattern sub-class for the replay functionality.
 *
 * @details This file sub-classes HwchHorizontalLinePattern to provide the
 * Replay mechanism with a pattern that can be updated on-demand.
 *
 *****************************************************************************/

#ifndef __HwchReplayPattern_h__
#define __HwchReplayPattern_h__

#include "HwchLayer.h"
#include "HwchPattern.h"

namespace Hwch
{

    class ReplayPattern : public HorizontalLinePtn
    {
        bool mFrameNeedsUpdate = true;

        public:

            /**
             * Default constructor - defaults to 60Hz update frequency with a black
             * line on a white background.
             */
            ReplayPattern(uint32_t bgColour = eWhite,
                uint32_t fgColour = eBlack, float mUpdateFreq = 60.0) :
                HorizontalLinePtn(mUpdateFreq, fgColour, bgColour) {};

            /** Default destructor. */
            ~ReplayPattern() = default;

            /** The pattern is copyable constructible. */
            ReplayPattern(const ReplayPattern& rhs) = default;

            /** Disable move semantics - no dynamic state. */
            ReplayPattern(ReplayPattern&& rhs) = delete;

            /** Pattern is copy assignable. */
            ReplayPattern& operator=(const ReplayPattern& rhs) = default;

            /** Disable move semantics - no dynamic state. */
            ReplayPattern& operator=(const ReplayPattern&& rhs) = delete;

            /**
             * Returns a flag to signify whether the frame should be updated (i.e.
             * typically in response to the buffers being rotated (and so the next
             * buffer needs filling).
             */
            bool FrameNeedsUpdate()
            {
                if (mFrameNeedsUpdate)
                {
                    mUpdatedSinceFBComp = true;
                    mFrameNeedsUpdate = false;
                    return true;
                }

                return false;
            }

            /** Forces an update the next time the layer is sent */
            void ForceUpdate()
            {
                mFrameNeedsUpdate = true;
            }
    };
}

#endif // __HwchReplayPattern_h__
