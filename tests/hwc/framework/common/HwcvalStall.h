/****************************************************************************

Copyright (c) Intel Corporation (2014).

DISCLAIMER OF WARRANTY
NEITHER INTEL NOR ITS SUPPLIERS MAKE ANY REPRESENTATION OR WARRANTY OR
CONDITION OF ANY KIND WHETHER EXPRESS OR IMPLIED (EITHER IN FACT OR BY
OPERATION OF LAW) WITH RESPECT TO THE SOURCE CODE.  INTEL AND ITS SUPPLIERS
EXPRESSLY DISCLAIM ALL WARRANTIES OR CONDITIONS OF MERCHANTABILITY OR
FITNESS FOR A PARTICULAR PURPOSE.  INTEL AND ITS SUPPLIERS DO NOT WARRANT
THAT THE SOURCE CODE IS ERROR-FREE OR THAT OPERATION OF THE SOURCE CODE WILL
BE SECURE OR UNINTERRUPTED AND HEREBY DISCLAIM ANY AND ALL LIABILITY ON
ACCOUNT THEREOF.  THERE IS ALSO NO IMPLIED WARRANTY OF NON-INFRINGEMENT.
SOURCE CODE IS LICENSED TO LICENSEE ON AN "AS IS" BASIS AND NEITHER INTEL
NOR ITS SUPPLIERS WILL PROVIDE ANY SUPPORT, ASSISTANCE, INSTALLATION,
TRAINING OR OTHER SERVICES.  INTEL AND ITS SUPPLIERS WILL NOT PROVIDE ANY
UPDATES, ENHANCEMENTS OR EXTENSIONS.

File Name:      HwcvalStall.h

Description:    Class definitions for Hwc validation Stall class.

Environment:

Notes:

****************************************************************************/
#ifndef __HwcvalStall_h__
#define __HwcvalStall_h__

// NOTE: HwcTestDefs.h sets defines which are used in the HWC and DRM stack.
// -> to be included before any other HWC or DRM header file.
#include "HwcTestDefs.h"
#include <utils/String8.h>
#include "HwcvalDebug.h"

namespace Hwcval
{
    enum StallType
    {
        eStallSetDisplay = 0,
        eStallDPMS,
        eStallSetMode,
        eStallSuspend,
        eStallResume,
        eStallHotPlug,
        eStallHotUnplug,
        eStallGemWait,
        eStallMax
    };

    class Stall
    {
    public:
        Stall();
        Stall(const char* configStr, const char* name);
        Stall(uint32_t us, double pct);
        Stall(const Stall& rhs);

        void Do(Hwcval::Mutex* mtx = 0);

    private:
        android::String8 mName;
        uint32_t mUs;
        double mPct;
        int mRandThreshold;
    };
} // namespace Hwcval



#endif // __HwcvalStall_h__
