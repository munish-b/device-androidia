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
* File Name:            HwchProtectedContentTests.h
*
* Description:          HWC Proteted Content Test class definitions
*
* Environment:
*
* Notes:
*
*****************************************************************************/

#ifndef __HwchProtectedContentTests_h__
#define __HwchProtectedContentTests_h__

#include "HwchTest.h"

namespace Hwch
{
    class ProtectedContentCrashTest : public OptionalTest
    {
        public:
            ProtectedContentCrashTest(Hwch::Interface& interface);

            virtual int RunScenario();
    };

    class ProtectedVideoTest  : public Test
    {
        public:

            ProtectedVideoTest(Hwch::Interface& interface);

            virtual int RunScenario();

    };

    class ProtectedVideoHotPlugTest  : public Test
    {
        public:

            ProtectedVideoHotPlugTest(Hwch::Interface& interface);

            virtual int RunScenario();

    };

    class ProtectedVideoScreenDisableTest  : public Test
    {
        public:

            ProtectedVideoScreenDisableTest(Hwch::Interface& interface);

            virtual int RunScenario();

    };

    class InvalidProtectedVideoTest  : public Test
    {
        public:

            InvalidProtectedVideoTest(Hwch::Interface& interface);

            virtual int RunScenario();

    };

    class ComplexProtectedVideoTest  : public Test
    {
        public:

            ComplexProtectedVideoTest(Hwch::Interface& interface);

            virtual int RunScenario();

    };
}

#endif // __HwchProtectedContentTests_h__

