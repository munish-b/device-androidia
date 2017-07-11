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
 * @file    ReplayDSRunner.h
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    12th June 2014
 * @brief   Header for dumpsys replay runner.
 *
 * @details Top-level header for replaying scenarios based on files generated
 * by the dumpsys command (hence DS prefix). It is safe to pass entire dumpsys
 * files to the runner - only the SurfaceFlinger section will be parsed.
 *
 *****************************************************************************/

#ifndef __HwchReplayDSRunner_h__
#define __HwchReplayDSRunner_h__

#include "HwchInterface.h"
#include "HwchReplayRunner.h"
#include "HwchReplayParser.h"

namespace Hwch
{

    class ReplayDSRunner : public ReplayRunner
    {
        private:

            /** Type for cacheing pointers to dynamically allocated layers. */
            using layer_cache_t = android::Vector<android::sp<ReplayLayer>>;

            /* Utility function to process layers outside of the main loop */
            bool AddLayers(Hwch::Frame& frame, int32_t display,
                           layer_cache_t& layer_cache);

            /* The number of frames to replay (specified on the command-line) */
            int32_t mNumFrames;

            /**
             * Statistics structure to count the following events:
             *
             *   parsed_frame_count - number of frames parsed
             *   parsed_layer_count - number of layers parsed (in total).
             *   hwc_frame_count - number of frames sent to the HWC.
             *   processed_layer_count - number of layers that are not
             *                           framebuffer targets
             */
            struct statistics
            {
                uint32_t parsed_frame_count = 0;
                uint32_t parsed_layer_count = 0;
                uint32_t hwc_frame_count = 0;
                uint32_t processed_layer_count = 0;
            } mStats;

        public:

            /**
             * No default constructor. There is currently no mechanism for
             * instantiating a ReplayDSRunner and then subsequently passing
             * in a file to parse.
             */
            ReplayDSRunner() = delete;

            /** Default destructor. */
            ~ReplayDSRunner() = default;

            /** DSRunner is not copyable (see default constructor). */
            ReplayDSRunner(const ReplayRunner& rhs) = delete;

            /** Disable move semantics (see default constructor). */
            ReplayDSRunner(ReplayRunner&& rhs) = delete;

            /** DSRunner is not copyable (see default constructor). */
            ReplayDSRunner& operator=(const ReplayDSRunner& rhs) = delete;

            /** Disable move semantics (see default constructor). */
            ReplayDSRunner& operator=(const ReplayDSRunner&& rhs) = delete;

            /**
             * @name  ReplayDSRunner
             * @brief Main constructor for running a dumpsys scenario.
             *
             * @param interface  Reference to the Hardware Composer interface.
             * @param filename   File to replay (typically from command line).
             * @param num_frames Number of frames to replay.
             *
             * @details This is the main user constructor for replaying dumpsys
             * scenarios. Note, if the file can not be opened (or is empty) the
             * program sets a status flag which can be tested at the top-level.
             */
            ReplayDSRunner(Hwch::Interface& interface, const char *filename,
                int32_t num_frames) :
                ReplayRunner(interface, filename),
                mNumFrames(num_frames)
                {};

            /**
             * @name  PrintStatistics
             * @brief Outputs statistics relating to the dumpsys snapshot replay.
             *
             * @details Calls printf to display statistics relating to the dumpsys
             * snapshot replay.
             */
            void PrintStatistics(void) override;

            /**
             * @name  RunScenario
             * @brief Top-level function to run the replay.
             *
             * @retval true      Scenario was replayed sucessfully.
             * @retval false     An error occurred (check logs).
             *
             * @details This is the entry point to the dumpsys runner. Calling
             * this function runs the replay scenario.
             */
            int RunScenario(void) override;
    };
}

#endif // __HwchReplayDSRunner_h__
