/****************************************************************************

Copyright (c) Intel Corporation (2015).

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

File Name:      HwcvalStatistics.h

Description:    Class definitions for statistics gathering.

Environment:

Notes:

****************************************************************************/

#include "HwcvalStatistics.h"
#include "HwcTestState.h"

namespace android
{
    ANDROID_SINGLETON_STATIC_INSTANCE(Hwcval::Statistics);
}

namespace Hwcval
{
    Statistics::Statistic::Statistic(const char* name)
      : mName(name)
    {
        Statistics::getInstance().Register(this);
    }

    Statistics::Statistic::~Statistic()
    {
    }

    const char* Statistics::Statistic::GetName()
    {
        return mName.string();
    }

    Statistics::Counter::Counter(const char* name)
      : Statistic(name),
        mCount(0)
    {
    }

    Statistics::Counter::~Counter()
    {
    }

    void Statistics::Counter::Inc()
    {
        ++mCount;
    }

    void Statistics::Counter::Clear()
    {
        mCount = 0;
    }

    void Statistics::Counter::Dump(FILE* file, const char* prefix)
    {
        fprintf(file, "%s,%s,0,%d\n", prefix, GetName(), mCount);
    }

    uint32_t Statistics::Counter::GetValue()
    {
        return mCount;
    }

    // Statistics methods
    void Statistics::Register(Statistic* stat)
    {
        mStats.add(stat);
    }

    void Statistics::Dump(FILE* file, const char* prefix)
    {
        for (uint32_t i=0; i<mStats.size(); ++i)
        {
            Statistic* stat = mStats.itemAt(i);
            stat->Dump(file, prefix);
        }
    }

    void Statistics::Clear()
    {
        for (uint32_t i=0; i<mStats.size(); ++i)
        {
            Statistic* stat = mStats.itemAt(i);
            stat->Clear();
        }
    }

    // Histogram
    Statistics::Histogram::Histogram(const char* name, bool cumulative)
      : Aggregate<uint32_t>(name),
        mCumulative(cumulative)
    {
    }

    Statistics::Histogram::~Histogram()
    {
    }

    void Statistics::Histogram::Add(uint32_t measurement)
    {
        Aggregate<uint32_t>::Add(measurement);

        for (uint32_t i=mElement.size(); i<=measurement; ++i)
        {
            mElement.add(0);
        }

        mElement.editItemAt(measurement)++;
    }

    void Statistics::Histogram::Clear()
    {
        Aggregate<uint32_t>::Clear();
        mElement.clear();
    }

    void Statistics::Histogram::Dump(FILE* file, const char* prefix)
    {
        Aggregate<uint32_t>::Dump(file, prefix);

        if (mCumulative)
        {
            uint32_t runningTotal = 0;

            for (uint32_t i=0; i<mElement.size(); ++i)
            {
                runningTotal += mElement.itemAt(i);
                fprintf(file, "%s,%s_cf,%d,%d\n", prefix, GetName(), i, runningTotal);
            }
        }
        else
        {
            for (uint32_t i=0; i<mElement.size(); ++i)
            {
                fprintf(file, "%s,%s_v,%d,%d\n", prefix, GetName(), i, mElement.itemAt(i));
            }
        }
    }


} // namespace Hwcval

