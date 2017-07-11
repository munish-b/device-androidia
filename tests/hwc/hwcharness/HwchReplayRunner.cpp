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
 * @file    Hwch::ReplayRunner.cpp
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    12th June 2014
 * @brief   Implementation of the Replay runner base class functionality.
 *
 * @details This file implements the top-level of the runner functionality.
 *
 *****************************************************************************/

#include "HwchInterface.h"
#include "HwcTestLog.h"
#include "HwcTestState.h"

#include "HwchReplayRunner.h"

Hwch::ReplayRunner::ReplayRunner(Hwch::Interface& interface, const char *filename)
    : Hwch::Test(interface), mInterface(interface)
{
    // Open the file and initialise the parser
    mFile.open(filename);
    mParser = new Hwch::ReplayParser();

    if (!mFile.good())
    {
        HWCERROR(eCheckReplayFail, "Fatal error opening replay file");
    }
    else if (!mParser->IsReady())
    {
        HWCERROR(eCheckReplayFail, "Replay parser not ready");
    }
    else
    {
        mReplayReady = true;
    }
}
