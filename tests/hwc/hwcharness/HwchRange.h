/****************************************************************************
*
* Copyright (c) Intel Corporation (2015).
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
* File Name:            HwchRange.h
*
* Description:          HWC Harness Range class definition
*                       Command-line range specification
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#ifndef __HwchRange_h__
#define __HwchRange_h__

#include "HwchChoice.h"
#include "HwcvalSelector.h"

namespace Hwch
{
    class Subrange : public android::RefBase
    {
    public:
        virtual ~Subrange();
        virtual bool Test(int32_t value) = 0;
    };

    class SubrangeContiguous : public Subrange
    {
    public:
        SubrangeContiguous(int32_t start, int32_t end);
        virtual ~SubrangeContiguous();
        virtual bool Test(int32_t value);

    private:
        int32_t mStart;
        int32_t mEnd;
    };

    class SubrangePeriod : public Subrange
    {
    public:
        SubrangePeriod(uint32_t interval);
        virtual ~SubrangePeriod();
        virtual bool Test(int32_t value);

    private:
        uint32_t mInterval;
    };

    class SubrangeRandom : public Subrange
    {
    public:
        SubrangeRandom(uint32_t interval);
        virtual ~SubrangeRandom();
        virtual bool Test(int32_t value);

    private:
        Choice mChoice;
    };

    class Range : public Hwcval::Selector
    {
    public:
        Range();
        Range(int32_t mn, int32_t mx);

        // Range specification is a comma-separated list of subranges being either:
        // a. number <n>
        // b. contiguous subrange [<m>]-[<n>]
        //    e.g. 23-46 OR -500 OR 200-
        // c. period <x>n e.g. 2n to indicate every second instance
        // d. randomized period e.g. 2r to indicate every second instance on average.
        Range(const char* spec);

        // Add a subrange to the range
        void Add(Subrange* subrange);

        // return true if the number is in the range
        virtual bool Test(int32_t n);

    private:
        // list of subranges
        android::Vector<android::sp<Subrange> > mSubranges;
    };
}

#endif // __HwchRange_h__

