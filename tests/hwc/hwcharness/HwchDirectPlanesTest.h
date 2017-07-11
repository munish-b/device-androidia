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
* File Name:            HwchRandomModesTest.h
*
* Description:          HWC API Test class definition
*
* Environment:
*
* Notes:
*
*****************************************************************************/

#ifndef __HwchDirectPlanesTest_h__
#define __HwchDirectPlanesTest_h__

#include "HwchRandomTest.h"
#include "HwchLayerChoice.h"

namespace Hwch
{
    class DirectPlanesTest  : public RandomTest
    {
    public:

        DirectPlanesTest(Hwch::Interface& interface);
        virtual ~DirectPlanesTest();

        virtual int RunScenario();

        bool IsFullScreen(const Hwch::LogDisplayRect& ldr, uint32_t d);
        Layer* CreateLayer(uint32_t d);
        void SetLayerCropDf(Hwch::Layer* layer, uint32_t d);
        void SetLayerFullScreen(Hwch::Layer* layer, uint32_t d);
        void SetLayerBlending(Hwch::Layer* layer);
        void SetLayerTransform(Hwch::Layer* layer);

    private:
        int32_t mDw[MAX_DISPLAYS];
        int32_t mDh[MAX_DISPLAYS];

        MultiChoice<uint32_t> mColourChoice;
        Choice mWidthChoice[MAX_DISPLAYS];
        Choice mHeightChoice[MAX_DISPLAYS];
        MultiChoice<uint32_t> mTransformChoice;
        MultiChoice<uint32_t> mBlendingChoice;
        AlphaChoice mAlphaChoice;
    };
}

#endif // __HwchDirectPlanesTest_h__

