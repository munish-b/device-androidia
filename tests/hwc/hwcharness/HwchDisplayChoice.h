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

#ifndef __HwchLayerChoice_h__
#define __HwchApiTest_h__

#include "HwchChoice.h"
#include "HwchSystem.h"

namespace Hwch
{
    class WidiResolutionChoice : public GenericChoice<uint32_t>
    {
    public:
        WidiResolutionChoice();
        ~WidiResolutionChoice() = default;

        virtual uint32_t Get();
        uint32_t NumChoices() { return eLastEntry; }
        uint32_t GetWidth() { return mWidth; }
        uint32_t GetHeight() { return mHeight; }

    private:
        // Note: enum values MUST be sequential and not reordered
        enum WidiResolutionType
        {
            eSameAsPanel = 0,
            eFixed1080p,
            eFixed720p,
            eRandom,
            eLastEntry
        };

        MultiChoice<WidiResolutionType> mWidiResolutionChoice;
        uint32_t mWidth;
        uint32_t mHeight;
    };

    class WidiFenceReleasePolicyChoice : public GenericChoice<uint32_t>
    {
    public:
        WidiFenceReleasePolicyChoice();
        ~WidiFenceReleasePolicyChoice() = default;

        virtual uint32_t Get();
        uint32_t NumChoices() { return Hwch::System::FenceReleaseMode::eLastEntry; }

    private:

        MultiChoice<Hwch::System::FenceReleaseMode> mWidiFenceReleasePolicyChoice;
    };
}

#endif // __HwchDisplayChoice_h__
