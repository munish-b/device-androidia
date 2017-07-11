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
 * @file    ReplayHWCLRunner.h
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    12th June 2014
 * @brief   Header for HWC log-file replay runner.
 *
 * @details Top-level header for replaying scenarios based on HWC log files.
 *
 *****************************************************************************/

#ifndef __HwchReplayHWCLRunner_h__
#define __HwchReplayHWCLRunner_h__

#include "HwchInterface.h"

#ifdef HWCVAL_BUILD_PAVP
#include "libpavp.h"
#endif

#include "HwchReplayRunner.h"
#include "HwchReplayParser.h"

/** Flag to enable debug code in the HWCL Replay Runner. */
#define REPLAY_HWCL_DEBUG 1

namespace Hwch
{

    class ReplayHWCLRunner : public ReplayRunner
    {
        private:

            /** Layer cache key type. */
            using layer_cache_key_t = uint32_t;

            /** Type for mapping buffer handles to dynamically allocated layers. */
            using layer_cache_t = android::KeyedVector< layer_cache_key_t,
                android::sp<ReplayLayer>>;

            /** Data structure for mapping parsed session/instances to live ones. */
    #ifdef HWCVAL_BUILD_PAVP
            struct prot_session
            {
                int32_t parsed_session = -1;            // Parsed session id
                int32_t parsed_instance = -1;           // Parsed instance id
                int32_t active_session = -1;            // Active session id
                int32_t active_instance = -1;           // Active instance id
            } mProtectedSessions[HWCVAL_MAX_PROT_SESSIONS];
    #endif

            /** Constants for number of nano/millseconds in a second. */
            const uint64_t mNanosPerSec = 1000000000;
            const uint64_t mMillisPerSec = 1000000;

            /** Skip inter-frame delays that are larger than mInterframeBound (seconds). */
            int32_t mInterframeBound = 60;

            /** Flag to store the match algorithm to use in the buffer tracking. */
            uint32_t mReplayMatch = 0;

            /** Command line argument to disable inter-frame spacing. */
            bool mReplayNoTiming = false;

            /** Command line argument to disable protected content. */
            bool mReplayNoProtected = false;

            /** Command line argument to set the alpha value. */
            int32_t mAlpha = 0xFF;

            /** Function to create a layer cache key from an index and a display. */
            inline layer_cache_key_t MakeLayerCacheKey(uint32_t layer_index,
                uint32_t display)
            {
                return (layer_index << 4) | display;
            }

            /** Utility function to process layers outside of the main loop. */
            bool AddLayers(Frame& frame, uint32_t display,
                layer_cache_t& layer_cache,
                layer_cache_t& prev_layer_cache,
                int32_t secs, int32_t msecs);

            /**
             * Statistics structure to count the following events:
             *
             *   parsed_onset_count - number of 'onSet Entry' statements parsed
             *   parsed_layer_count - number of layers parsed (in total).
             *   skip_layer_count - number of skip layers parsed.
             *   hwc_frame_count - number of frames sent to the HWC.
             *   processed_layer_count - number of layers excluding framebuffer
             *                           targets and layers with unsupported colour
             *                           spaces (i.e. '???').
             *   encrypted_layer_count - number of encrypted layers parsed
             *   match_count - number of layers matched by the buffer tracking.
             *   allocation_count - total number of buffer allocations
             *   hotplug_count - total number of hotplug events
             *   hotplug_connects_count - number of hotplug connects
             *   hotplug_disconnects_count - number of hotplug disconnects
             *   blanking_count - total number of blanking events
             *   blanking_blank_count - number of 'blank' events
             *   blanking_unblank_count - number of 'unblank' events
             */
            struct statistics
            {
                uint32_t parsed_onset_count = 0;
                uint32_t parsed_layer_count = 0;
                uint32_t skip_layer_count = 0;
                uint32_t hwc_frame_count = 0;
                uint32_t processed_layer_count = 0;
                uint32_t encrypted_layer_count = 0;
                uint32_t match_count = 0;
                uint32_t allocation_count = 0;
                uint32_t hotplug_count = 0;
                uint32_t hotplug_connects_count = 0;
                uint32_t hotplug_disconnects_count = 0;
                uint32_t blanking_count = 0;
                uint32_t blanking_blank_count = 0;
                uint32_t blanking_unblank_count = 0;
            } mStats;

        public:

            /**
             * No default constructor. There is currently no mechanism for
             * instantiating a ReplayHWCLRunner and then subsequently passing
             * in a file to parse.
             */
            ReplayHWCLRunner() = delete;

            /** HWCLRunner is not copyable (see default constructor). */
            ReplayHWCLRunner(const ReplayRunner& rhs) = delete;

            /** Disable move semantics (see default constructor). */
            ReplayHWCLRunner(ReplayRunner&& rhs) = delete;

            /** HWCLRunner is not copyable (see default constructor). */
            ReplayHWCLRunner& operator=(const ReplayHWCLRunner& rhs) = delete;

            /** Disable move semantics (see default constructor). */
            ReplayHWCLRunner& operator=(const ReplayHWCLRunner&& rhs) = delete;
            ~ReplayHWCLRunner() = default;

            /**
             * @name  ReplayHWCLRunner
             * @brief Main constructor for replaying a scenario based on HWC logs.
             *
             * @param interface         Reference to the Hardware Composer interface.
             * @param filename          File to replay (typically from command line).
             * @param replayMatch       Integer to select the buffer tracking algorithm.
             * @param replayNoTiming    Flag to disable inter-frame spacing
             * @param replayNoProtected Flag to disable inter-frame spacing
             * @param alpha             Alpha value to apply to each layer
             *
             * @details This is the main user constructor for replaying HWC log
             * scenarios. Note, if the file can not be opened (or is empty) the
             * program sets a status flag which can be tested from the top-level.
             */
            ReplayHWCLRunner(Hwch::Interface& interface, const char *filename,
                uint32_t replayMatch, bool replayNoTiming, bool replayNoProtected, int32_t alpha)
              : ReplayRunner(interface, filename),
                mReplayMatch(replayMatch),
                mReplayNoTiming(replayNoTiming),
                mReplayNoProtected(replayNoProtected),
                mAlpha(alpha)
                {};

            /**
             * @name  PrintStatistics
             * @brief Outputs statistics relating to the HWC log replay.
             *
             * @details Calls printf to display statistics relating to the HWC log replay.
             */
            void PrintStatistics(void) override;

            /**
             * @name  RunScenario
             * @brief Top-level function to run the replay.
             *
             * @retval true      Scenario was replayed sucessfully.
             * @retval false     An error occurred (check logs).
             *
             * @details This is the entry point to the HWC log runner. Calling
             * this function runs the replay scenario.
             */
            int RunScenario(void) override;

    #ifdef HWCVAL_BUILD_PAVP
            /**
             * @name  DisableOneOrAllProtectedSessions
             * @brief Disables a specified protected session or all of them
             *
             * @param pavp_session PAVP session handle.
             * @param session      Can be a valid session id or '-1' to indicate that
             *                     all sessions should be destroyed.
             */
            void DisableOneOrAllProtectedSessions(AbstractPavpSession& pavp_session, int32_t session);
    #endif // HWCVAL_BUILD_PAVP
    };
}

#endif // __HwchReplayHWCLRunner_h__
