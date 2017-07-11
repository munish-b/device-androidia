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
* File Name:            HwchChoice.cpp
*
* Description:          Implementation of HWC basic Choice classes
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#include "HwchChoice.h"
#include "HwcTestState.h"
#include "HwcTestUtil.h"
#include <math.h>

static const float delta = 0.0001;

// Choice
Hwch::Choice::Choice(int mn, int mx, const char* name)
  : mMin(mn),
    mMax(mx),
    mName(name)
{
    HWCLOGV_COND(eLogHarness, "Choice::Choice(%d, %d, %s)", mn, mx, name);
}

Hwch::Choice::~Choice()
{
}

void Hwch::Choice::Setup(int mn, int mx, const char* name)
{
    mMin = mn;
    mMax = mx;
    mName = name;
    HWCLOGV_COND(eLogHarness, "Choice::Setup(%d, %d, %s)", mn, mx, name);
}

int Hwch::Choice::Get()
{
    // TODO: Support systematic non-repeating forms of choice
    if (mMax < mMin)
    {
        HWCLOGE("Hwch::Choice::Get(%s) mMin=%d mMax=%d", mName, mMin, mMax);
        ALOG_ASSERT(mMax >= mMin);
    }

    return ((rand() % (mMax + 1 - mMin))  + mMin);
}

void Hwch::Choice::IncMax()
{
    ++mMax;
}

void Hwch::Choice::SetMin(int mn)
{
    mMin = mn;
}

void Hwch::Choice::SetMax(int mx, bool disable)
{
    if (disable)
    {
        mMax = mMin - 1;
    }
    else
    {
        mMax = mx;
        if (mMax < mMin)
        {
            ALOGE("Choice::SetMax min=%d max=%d name=%s", mMin, mMax, mName);
            ALOG_ASSERT(mMax >= mMin);
        }
    }
}

uint32_t Hwch::Choice::NumChoices()
{
    return mMax - mMin + 1;
}

void Hwch::Choice::Seed(uint32_t seed)
{
    HWCLOGA("Hwch::Choice::Seed(%d)", seed);
    srand(seed);
}

bool Hwch::Choice::IsEnabled()
{
    return (mMax >= mMin);
}

// FLoatChoice
Hwch::FloatChoice::FloatChoice(float mn, float mx, const char* name)
  : mMin(mn),
    mMax(mx),
    mName(name)
{
    if (mMax < mMin)
    {
        mMax = mMin;
    }
}

Hwch::FloatChoice::~FloatChoice()
{
}

float Hwch::FloatChoice::Get()
{
    // TODO: Support systematic non-repeating forms of choice
    return (((float(rand()) / RAND_MAX) * (mMax - mMin))  + mMin);
}

void Hwch::FloatChoice::SetMax(float mx)
{
    mMax = mx;
}

uint32_t Hwch::FloatChoice::NumChoices()
{
    // Theoretically infinite. Practically limited by floating point representation.
    return 1000;
}

// LogarithmicChoice
// An alternative to FloatChoice with a logarithmic distribution.
// This means that small values are more likely to be returned than large ones
// for instance the probability of a value between x and x+1 is the same as
// that of a value between 2x and 2x+2.
Hwch::LogarithmicChoice::LogarithmicChoice(double mn, double mx, const char* name)
  : mChoice(log(mn), log(mx), name)
{
    if (mx < mn)
    {
        SetMax(mn);
    }
}

Hwch::LogarithmicChoice::~LogarithmicChoice()
{
}

double Hwch::LogarithmicChoice::Get()
{
    // TODO: Support systematic non-repeating forms of choice
    return exp(mChoice.Get());
}

void Hwch::LogarithmicChoice::SetMax(double mx)
{
    mChoice.SetMax(log(mx));
}

uint32_t Hwch::LogarithmicChoice::NumChoices()
{
    // Theoretically infinite. Practically limited by floating point representation.
    return 1000;
}

// LogIntChoice
// An integer choice clas with a logarithmic distribution
// so that small values are more likely than large ones.
Hwch::LogIntChoice::LogIntChoice(uint32_t mn, uint32_t mx, const char* name)
  : mLogChoice(max(double(mn), 0.1), double(mx + 1) - delta, name),
    mMin(mn),
    mMax(mx)
{
    HWCLOGV_COND(eLogHarness, "Choice::LogIntChoice(%d, %d)", mn, mx);
}

Hwch::LogIntChoice::~LogIntChoice()
{
}

uint32_t Hwch::LogIntChoice::Get()
{
    return uint32_t(mLogChoice.Get());
}

void Hwch::LogIntChoice::SetMax(uint32_t mx)
{
    mMax = mx;
    mLogChoice.SetMax(double(mx + 1) - delta);
}

uint32_t Hwch::LogIntChoice::NumChoices()
{
    return mMax + 1 - mMin;
}

Hwch::EventDelayChoice::EventDelayChoice(uint32_t mx, const char* name)
  : mDelayChoice(0, mx, name),
    mSyncChoice(0, 1)
{
}

Hwch::EventDelayChoice::~EventDelayChoice()
{
}

void Hwch::EventDelayChoice::SetMax(int mx)
{
    if (mx < 0)
    {
        // Only allow synchronous
        mSyncChoice.SetMin(1);
    }
    else
    {
        mSyncChoice.SetMin(0);
        mDelayChoice.SetMax(mx);
    }
}

int Hwch::EventDelayChoice::Get()
{
    if (mSyncChoice.Get())
    {
        // Synchronous, no event delay
        return -1;
    }
    else
    {
        return int(mDelayChoice.Get());
    }
}

uint32_t Hwch::EventDelayChoice::NumChoices()
{
    return mDelayChoice.NumChoices() + 1;
}


