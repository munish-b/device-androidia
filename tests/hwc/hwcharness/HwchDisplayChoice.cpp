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
* File Name:            HwchApiTest.cpp
*
* Description:          Implementation of HWC complex choice classes
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#include "HwchDisplayChoice.h"
#include "HwcTestState.h"
#include "HwcTestUtil.h"
#include "HwchSystem.h"

// Implement choice class to determine resolution sizes for Widi connections
Hwch::WidiResolutionChoice::WidiResolutionChoice()
{
  // Add the different options to the choice class - keep probabilities constant
  for (uint32_t i = eSameAsPanel; i != eLastEntry; ++i)
  {
    mWidiResolutionChoice.Add(static_cast<WidiResolutionType>(i));
  }
}

uint32_t Hwch::WidiResolutionChoice::Get()
{
    uint32_t res = mWidiResolutionChoice.Get();

    int32_t panel_width = Hwch::System::getInstance().GetDisplay(0).GetWidth();
    int32_t panel_height = Hwch::System::getInstance().GetDisplay(0).GetHeight();

    switch(res)
    {
        case eSameAsPanel:
        {
            int32_t mWidth = panel_width;
            int32_t mHeight = panel_height;
            HWCLOGV_COND(eLogHarness, "Setting Widi width/height to %d %d\n", mWidth, mHeight);
            return eSameAsPanel;
        }
        case eFixed1080p:
        {
            mWidth = 1920; mHeight = 1080;
            HWCLOGV_COND(eLogHarness, "Setting Widi width/height to %d %d\n", mWidth, mHeight);
            return eFixed1080p;
        }
        case eFixed720p:
        {
            mWidth = 1280; mHeight = 720;
            return eFixed720p;
        }
        case eRandom:
        {
            Hwch::Choice width_choice(panel_width / 2, panel_width);
            Hwch::Choice height_choice(panel_height / 2, panel_height);
            mWidth = width_choice.Get(); mHeight = height_choice.Get();
            HWCLOGV_COND(eLogHarness, "Setting Widi width/height to %d %d\n", mWidth, mHeight);
            return eRandom;
        }
        default:
            ALOG_ASSERT(0);
            return 0;
    }
}

// Implement choice class to determine fence release policy for Widi
Hwch::WidiFenceReleasePolicyChoice::WidiFenceReleasePolicyChoice()
{
  using FenceReleaseMode = Hwch::System::FenceReleaseMode;

  // Add the different options to the choice class - keep probabilities constant
  for (uint32_t i = FenceReleaseMode::eSequential; i != FenceReleaseMode::eLastEntry; ++i)
  {
    mWidiFenceReleasePolicyChoice.Add(static_cast<FenceReleaseMode>(i));
  }
}

// Returns a random Widi fence release policy
uint32_t Hwch::WidiFenceReleasePolicyChoice::Get()
{
  return mWidiFenceReleasePolicyChoice.Get();
}
