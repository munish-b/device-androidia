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
* File Name:            HwchFlickerTests.h
*
* Description:          HWC Flicker tests definition
*
* Environment:
*
* Notes:
*
*****************************************************************************/

#ifndef __HwchFlickerTests_h__
#define __HwchFlickerTests_h__

#include "HwchTest.h"

namespace Hwch
{


    class Flicker1Test  : public Test
    {
        public:

            Flicker1Test(Hwch::Interface& interface);

            virtual int RunScenario();

    };

    class Flicker2Test  : public Test
    {
        public:

            Flicker2Test(Hwch::Interface& interface);

            virtual int RunScenario();

    };

    class Flicker3Test  : public Test
    {
        public:

            Flicker3Test(Hwch::Interface& interface);

            virtual int RunScenario();

    };

}

#endif // __HwchFlickerTests_h__

