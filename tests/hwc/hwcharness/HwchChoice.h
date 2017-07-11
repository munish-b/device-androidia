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
* File Name:            HwchChoice.h
*
* Description:          HWC basic Choice classes definitions
*
* Environment:
*
* Notes:
*
*****************************************************************************/

#ifndef __HwchChoice_h__
#define __HwchChoice_h__

#include <utils/Vector.h>
#include <utils/RefBase.h>

namespace Hwch
{
    // Choice class to make it generic to choose between options.
    template<class C>
    class GenericChoice : public android::RefBase
    {
    public:
        GenericChoice<C>()
        {
        }

        virtual ~GenericChoice<C>()
        {
        }

        // Return a choice
        virtual C Get() = 0;

        // How many valid choices are there?
        virtual uint32_t NumChoices() = 0;

        // How many iterations should we do? Default to same as number of choices
        virtual uint32_t NumIterations()
        {
            return NumChoices();
        }

        virtual bool IsEnabled()
        {
            return (NumChoices() > 0);
        }
    };

    class Choice : public GenericChoice<int>
    {
    public:
        Choice(int mn=0, int mx=0, const char* name="Choice");
        virtual ~Choice();
        void IncMax();
        void Setup(int mn=0, int mx=0, const char* name="Choice");
        void SetMax(int mx, bool disable = false);
        void SetMin(int mn);
        virtual int Get();
        virtual uint32_t NumChoices();
        static void Seed(uint32_t seed);
        virtual bool IsEnabled();

    protected:
        int mMin;
        int mMax;
        const char* mName;
    };

    class FloatChoice : public GenericChoice<float>
    {
    public:
        FloatChoice(float mn=0, float mx=0, const char* name="Float");
        virtual ~FloatChoice();
        void SetMax(float mx);
        virtual float Get();
        virtual uint32_t NumChoices();

    protected:
        float mMin;
        float mMax;
        const char* mName;
    };

    class LogarithmicChoice : public GenericChoice<double>
    {
    public:
        LogarithmicChoice(double mn=0, double mx=0, const char* name="Logarithmic");
        virtual ~LogarithmicChoice();
        void SetMax(double mx);
        virtual double Get();
        virtual uint32_t NumChoices();

    protected:
        FloatChoice mChoice;
    };

    class LogIntChoice : public GenericChoice<uint32_t>
    {
    public:
        LogIntChoice(uint32_t mn=0, uint32_t mx=0, const char* name="LogInt");
        virtual ~LogIntChoice();
        void SetMax(uint32_t mx);
        virtual uint32_t Get();
        virtual uint32_t NumChoices();

    protected:
        LogarithmicChoice mLogChoice;
        uint32_t mMin;
        uint32_t mMax;
    };

    class EventDelayChoice: public GenericChoice<int>
    {
    public:
        EventDelayChoice(uint32_t mx, const char* name="EventDelay");
        virtual ~EventDelayChoice();
        void SetMax(int mx);
        virtual int Get();
        virtual uint32_t NumChoices();

    private:
        LogIntChoice mDelayChoice;
        Choice mSyncChoice;
    };

    template<class C>
    class MultiChoice : public GenericChoice<C>
    {
    public:
        MultiChoice<C>(const char* name="MultiChoice")
          : GenericChoice<C>(),
            c(0, -1, name)
        {
        }

        void Add(C option)
        {
            mOptions.add(option);
            c.IncMax();
        }

        virtual C Get()
        {
            return mOptions[c.Get()];
        }

        virtual uint32_t NumChoices()
        {
            return c.NumChoices();
        }

    private:
        Choice c;
        android::Vector<C> mOptions;
    };

}

#endif // __HwchChoice_h__

