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

#ifndef __HwchRandomModesTest_h__
#define __HwchRandomModesTest_h__

#include "HwchRandomTest.h"
#include "HwchLayers.h"

namespace Hwch
{
    class RandomModesTest  : public RandomTest
    {
    public:

        RandomModesTest(Hwch::Interface& interface);
        virtual ~RandomModesTest();

        void ChooseExtendedMode();
        void DetermineExtendedModeExpectation();
        virtual void ClearVideo();
        virtual int RunScenario();

    private:
        Choice mExtendedModeChooser;
        MultiChoice<uint32_t> mVideoRateChoice;

        // True if full-screen video layer is showing
        bool mVideoPresent;

        // True if MDS will report video playing
        bool mVideoPlaying;

        // True if MDS will report input timeout
        bool mInputTimeout;

        // Enable flag for workaround:
        // If we say "video is playing" during mode change or resume, this
        // may cause errors in validation of extended mode state.
        // To prevent this (not particularly realistic) condition happening, we have
        // this flag to stop the harness from doing this.
        bool mAvoidVideoOnResumeOrModeChange;
        uint32_t mDontStartExtendedModeBefore;

        NV12VideoLayer* mVideoLayer;
    };
}

#endif // __HwchRandomModesTest_h__

