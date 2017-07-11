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
* File Name:            HwchGlTests.h
*
* Description:          HWC Test class definitions
*
* Environment:
*
* Notes:
*
*****************************************************************************/

#ifndef __HwchGlTests_h__
#define __HwchGlTests_h__

#include "HwchTest.h"


namespace Hwch
{
    class PngGlLayer : public Layer
    {
        public:
            PngGlLayer(){};
            PngGlLayer(Hwch::PngImage& png, float updateFreq = 60.0, uint32_t lineColour = eWhite, uint32_t bgColour=0, bool bIgnore=false);
    };

    class GlBasicLineTest  : public OptionalTest
    {
        public:

            GlBasicLineTest(Hwch::Interface& interface);

            virtual int RunScenario();

    };

    class GlBasicClearTest  : public OptionalTest
    {
        public:

            GlBasicClearTest(Hwch::Interface& interface);

            virtual int RunScenario();

    };

    class GlBasicTextureTest  : public OptionalTest
    {
        public:

            GlBasicTextureTest(Hwch::Interface& interface);

            virtual int RunScenario();

    };

    class GlBasicCombo1Test  : public OptionalTest
    {
        public:

            GlBasicCombo1Test(Hwch::Interface& interface);

            virtual int RunScenario();

    };

    class GlBasicCombo2Test  : public OptionalTest
    {
        public:

            GlBasicCombo2Test(Hwch::Interface& interface);

            virtual int RunScenario();

    };

    class GlBasicCombo3Test  : public OptionalTest
    {
        public:

            GlBasicCombo3Test(Hwch::Interface& interface);

            virtual int RunScenario();

    };

    class GlBasicPixelDiscardTest  : public OptionalTest
    {
        public:

            GlBasicPixelDiscardTest(Hwch::Interface& interface);

            virtual int RunScenario();

    };

    class GlBasicViewportTest  : public OptionalTest
    {
        public:

            GlBasicViewportTest(Hwch::Interface& interface);

            virtual int RunScenario();

    };

    class GlBasicMovingLineTest  : public OptionalTest
    {
        public:

            GlBasicMovingLineTest(Hwch::Interface& interface);

            virtual int RunScenario();

    };

    class GlBasicPixelDiscardNOPTest  : public OptionalTest
    {
        public:

            GlBasicPixelDiscardNOPTest(Hwch::Interface& interface);

            virtual int RunScenario();

    };

}

#endif // __HwchGlTests_h__

