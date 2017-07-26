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

