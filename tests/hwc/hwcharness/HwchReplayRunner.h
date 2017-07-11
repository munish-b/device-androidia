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
 * @file    Replay.h
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    12th June 2014
 * @brief   Header for the replay functionality.
 *
 * @details Abstract base class for the 'runners' (e.g. ReplayHWCLRuner).
 * The rationale is to implement the runner functionality in derived classes
 * whilst providing a common runner interface to the system. This allows the
 * behaviour of the individual runners to vary whilst retaining encapsulation.
 *
 *****************************************************************************/

#ifndef __HwchReplayRunner_h__
#define __HwchReplayRunner_h__

#include <utils/RefBase.h>

#include "HwchInterface.h"
#include "HwchTest.h"
#include "HwchReplayLayer.h"
#include "HwchReplayParser.h"

namespace Hwch
{

    class ReplayRunner : public android::RefBase, public Test
    {
        protected:

            /** The base class owns the parser instance. */
            android::sp<ReplayParser> mParser;

            /** Reference to the HWC interface. */
            Hwch::Interface& mInterface;

            /** RAII file handle. */
            std::ifstream mFile;

            /**
             * Set to true if the replay file has been opened and the regex
             * compilation has been successful.
             */
            bool mReplayReady = false;

        public:

            /**
             * @name  ReplayRunner
             * @brief Base class constructor.
             *
             * @param interface  Reference to the Hardware Composer interface.
             * @param filename   File to replay (typically from command line).
             *
             * @details Handles file opening and dynamically allocates an instance
             * of the parser.
             */
            ReplayRunner(Hwch::Interface& interface, const char *filename);

            /** Returns whether the replay file was opened successfully. */
            bool IsReady()
            {
                return mReplayReady;
            }

            /** Virtual function to print individual 'runner' statistics. */
            virtual void PrintStatistics(void)
            {
                std::printf("No replay statistics implemented for this runner\n");
            }

            /** Empty virtual destructor. */
            virtual ~ReplayRunner() = default;

            /** Runs the regular expression unit tests for the parser. */
            bool RunParserUnitTests(void)
            {
                return mParser->RunParserUnitTests();
            }
    };
}

#endif // __HwchReplayRunner_h__
