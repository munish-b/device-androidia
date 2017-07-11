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

File Name:      HwcvalSelector.h

Description:    Class definitions for Hwc validation Selector class(es)

Environment:

Notes:

****************************************************************************/
#ifndef __HwcvalSelector_h__
#define __HwcvalSelector_h__

// NOTE: HwcTestDefs.h sets defines which are used in the HWC and DRM stack.
// -> to be included before any other HWC or DRM header file.
#include "HwcTestDefs.h"
#include "HwcTestUtil.h"
#include <utils/String8.h>
#include <utils/RefBase.h>

namespace Hwcval
{
    // Abstract selector class
    // Implementation should give a true or false which is dependent on the numeric input.
    // (This may not be entirely true for randomly based selectors).
    class Selector : public android::RefBase
    {
    public:
        Selector()
          : mValue(0)
        {
        }

        Selector(const Selector& rhs)
          : RefBase(),
            mValue(0)
        {
            HWCVAL_UNUSED(rhs);
        }

        Selector& operator=(const Selector& rhs)
        {
            // Value is not copied. Only the selection criteria which are in the subclass.
            HWCVAL_UNUSED(rhs);

            return *this;
        }

        // return true if the number is in the range
        virtual bool Test(int32_t n) = 0;

        // increment a counter, and return true if it is in the range
        bool Next();

    protected:
        // current value to test
        uint32_t mValue;
    };


    // increment a counter, and return true if it is in the range
    inline bool Selector::Next()
    {
        return Test(mValue++);
    }

} // namespace Hwcval



#endif // __HwcvalSelector_h__
