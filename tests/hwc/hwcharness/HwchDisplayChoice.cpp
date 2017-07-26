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
