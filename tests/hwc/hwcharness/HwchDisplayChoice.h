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
* File Name:            HwchDisplayChoice.h
*
* Description:          HWC display-related choice classes definitions
*
* Environment:
*
* Notes:
*
*****************************************************************************/

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
