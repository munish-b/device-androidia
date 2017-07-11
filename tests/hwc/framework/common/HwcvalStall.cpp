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

File Name:      HwcvalStall.cpp

Description:    Class implementation for Hwc validation Stall class.

Environment:

Notes:

****************************************************************************/

#include "HwcvalStall.h"
#include "HwcTestState.h"
#include "HwcTestUtil.h"

Hwcval::Stall::Stall()
  : mName("Undefined"),
    mUs(0),
    mPct(0),
    mRandThreshold(0)
{
}

// Constructor from a string like [<p>%]<d><unit>
// where <unit>=s|ms|us|ns
// order can also be reversed, i.e. delay first
// Percentage indicates percent of sample points where delay will take place.
// If omitted, delay will take place at all sample points.
Hwcval::Stall::Stall(const char* configStr, const char* name)
  : mName(name),
    mUs(0),
    mPct(100.0)
{
    // Parse string of the format [<x>%][<y><unit>]
    // where <unit>=s|ms|us|ns
    // and x and y are floating point

    const char* p = configStr;

    while (*p)
    {
        skipws(p);
        double n = atofinc(p);
        skipws(p);

        if (strncmpinc(p, "%") == 0)
        {
            mPct = n;
        }
        else if (strncmpinc(p, "s") == 0)
        {
            mUs = n * HWCVAL_SEC_TO_US;
        }
        else if (strncmpinc(p, "ms") == 0)
        {
            mUs = n * HWCVAL_MS_TO_US;
        }
        else if (strncmpinc(p, "us") == 0)
        {
            mUs = n;
        }
        else if (strncmpinc(p, "ns") == 0)
        {
            mUs = n / HWCVAL_US_TO_NS;
        }
        else
        {
            HWCLOGV_COND(eLogStall, "Stall::Stall %f NO MATCH %s", n, p);
        }
    }

    if (mUs == 0)
    {
        // Stall is disabled
        mPct = 0;
        mRandThreshold = 0;
    }
    else
    {
        mRandThreshold = RAND_MAX * mPct / 100.0;
    }

    HWCLOGD_COND(eLogStall, "Stall::Stall %s %s -> %f%% %fms threshold %d",
        name, configStr, mPct, double(mUs) / HWCVAL_MS_TO_US, mRandThreshold);
}

Hwcval::Stall::Stall(uint32_t us, double pct)
  : mName("Unknown"),
    mUs(us),
    mPct(pct),
    mRandThreshold(pct * RAND_MAX / 100.0)
{
}

Hwcval::Stall::Stall(const Stall& rhs)
  : mName(rhs.mName),
    mUs(rhs.mUs),
    mPct(rhs.mPct),
    mRandThreshold(rhs.mPct)
{
}

void Hwcval::Stall::Do(Hwcval::Mutex* mtx)
{
    HWCLOGV_COND(eLogStall, "Do %s threshold %d", mName.string(), mRandThreshold);
    if (mRandThreshold > 0)
    {
        if (rand() < mRandThreshold)
        {
            HWCLOGV_COND(eLogStall, "Executing %s stall %fms",
                mName.string(), double(mUs) / HWCVAL_MS_TO_US);

            if (mtx)
            {
                mtx->unlock();
            }

            usleep(mUs);

            if (mtx)
            {
                mtx->lock();
            }

            HWCLOGD_COND(eLogStall, "Completed %s stall %fms",
                mName.string(), double(mUs) / HWCVAL_MS_TO_US);
        }
    }
}

