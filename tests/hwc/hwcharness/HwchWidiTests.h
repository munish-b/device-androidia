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
 * @file    WidiTests.cpp
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    22nd April 2015
 * @brief   Implementation of the harness Widi tests
 *
 * @details This file implements the Widi harness tests.
 *
 *****************************************************************************/

#ifndef __HwchTests_h__
#define __HwchTests_h__

#include "HwchTest.h"

namespace Hwch
{
    class WidiTest : public Test
    {
        public:
            WidiTest(Hwch::Interface& interface)
                : Hwch::Test(interface) {};

            virtual bool IsAbleToRun()
            {
                return mSystem.IsWirelessDisplayEmulationEnabled();
            };
    };

    class WidiSmokeTest : public WidiTest
    {
        public:

            WidiSmokeTest(Hwch::Interface& interface);

            virtual int RunScenario();
    };

    class WidiDisconnectTest : public WidiTest
    {
        public:

            WidiDisconnectTest(Hwch::Interface& interface);

            virtual int RunScenario();
    };

    class WidiProcessingPolicyTest : public WidiTest
    {
        public:

            WidiProcessingPolicyTest(Hwch::Interface& interface);

            virtual bool IsAbleToRun() { return false; }

            virtual int RunScenario();
    };

    class WidiFrameTypeChangeTest : public WidiTest
    {
        public:

            WidiFrameTypeChangeTest(Hwch::Interface& interface);

            virtual bool IsAbleToRun() { return false; }

            virtual int RunScenario();
    };

    class WidiDirectTest : public WidiTest
    {
        public:

            WidiDirectTest(Hwch::Interface& interface);

            virtual int RunScenario();
    };

    class WirelessDockingTest : public OptionalTest
    {
        public:

            WirelessDockingTest(Hwch::Interface& interface);

            virtual int RunScenario();
    };
}

#endif // __HwchTests_h__

