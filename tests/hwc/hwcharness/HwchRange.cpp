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
* File Name:            HwchRange.cpp
*
* Description:          HWC Harness Range class implementation
*                       Command-line range specification
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#include "HwchRange.h"
#include "HwcTestUtil.h"
#include "HwcTestState.h"

namespace Hwch
{
    Subrange::~Subrange()
    {
    }

    SubrangeContiguous::SubrangeContiguous(int32_t start, int32_t end)
         : mStart(start), mEnd(end)
    {
    }

    SubrangeContiguous::~SubrangeContiguous()
    {
    }

    bool SubrangeContiguous::Test(int32_t value)
    {
        return ((value >= mStart) && (value <= mEnd));
    }

    SubrangePeriod::SubrangePeriod(uint32_t interval)
      : mInterval(interval)
    {
    }

    SubrangePeriod::~SubrangePeriod()
    {
    }

    bool SubrangePeriod::Test(int32_t value)
    {
        return ((value % mInterval) == 0);
    }

    SubrangeRandom::SubrangeRandom(uint32_t interval)
      : mChoice(interval)
    {
    }

    SubrangeRandom::~SubrangeRandom()
    {
    }

    bool SubrangeRandom::Test(int32_t value)
    {
        HWCVAL_UNUSED(value);
        return (mChoice.Get() == 0);
    }

    Range::Range()
    {
    }

    Range::Range(int32_t mn, int32_t mx)
    {
        Add(new SubrangeContiguous(mn, mx));
    }

    // Range specification is a comma-separated list of subranges being either:
    // a. number <n>
    // b. contiguous subrange [<m>]-[<n>]
    //    e.g. 23-46 OR -500 OR 200-
    // c. period <x>n e.g. 2n to indicate every second instance
    // d. randomized period e.g. 2r to indicate every second instance on average.
    Range::Range(const char* spec)
    {
        HWCLOGD("Constructing range %s", spec);
        const char* p = spec;
        while (*p)
        {
            int32_t v = INT_MIN;

            if (isdigit(*p))
            {
                v = atoiinc(p);
            }

            if (strncmpinc(p, "-") == 0)
            {
                int32_t v2 = INT_MAX;

                if (isdigit(*p))
                {
                    v2 = atoiinc(p);
                }

                HWCLOGD("Contiguous subrange %d-%d", v, v2);
                Add(new SubrangeContiguous(v, v2));
            }
            else if (strncmpinc(p, "n") == 0)
            {
                Add(new SubrangePeriod(v));
            }
            else if (strncmpinc(p, "r") == 0)
            {
                Add(new SubrangeRandom(v));
            }
            else if (*p == ',')
            {
                Add(new SubrangeContiguous(v,v));
            }

            if (strncmpinc(p, ",") != 0)
            {
                return;
            }
        }
    }

    void Range::Add(Subrange* subrange)
    {
        mSubranges.add(subrange);
    }

    bool Range::Test(int32_t value)
    {
        for (uint32_t i=0; i<mSubranges.size(); ++i)
        {
            android::sp<Subrange> subrange = mSubranges.itemAt(i);

            if (subrange->Test(value))
            {
                return true;
            }
        }

        return false;
    }
}
