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

File Name:      HwcvalLogIntercept.h

Description:    Hwc log intercept functions.

Environment:

Notes:

****************************************************************************/

#ifndef __Hwcval_LogIntercept_h__
#define __Hwcval_LogIntercept_h__

#include <cstdio>
#include <cstdarg>
#include <utils/Log.h>

#include "AbstractCompositionChecker.h"
#include "AbstractLog.h"

namespace Hwcval
{
    // Our abstract definition of a log checker.
    // Note that each checker has a DoParse function that must be implemented by the concrete subclass.
    // Log checkers can be chained together by passing a pointer to a log checker in the constructor to
    // its antecedent. A log checker will call the next one if and only if it fails to find a match
    // within its DoParse function.
    class LogChecker
    {
    public:
        LogChecker(LogChecker* next = 0)
          : mNext(next)
        {
        }

        virtual ~LogChecker()
        {
        }

        // To be implemented by the concrete subclass.
        // Must return true if it matches and consumes the log message, false otherwise.
        virtual bool DoParse(pid_t pid, int64_t timestamp, const char* str) = 0;

        // Call the DoParse function in this, and all following log checkers, until one of them matches the string.
        bool Parse(pid_t pid, int64_t timestamp, const char* str)
        {
            if (mNext)
            {
                if (mNext->Parse(pid, timestamp, str))
                {
                    return true;
                }
            }

            return DoParse(pid, timestamp, str);
        }

    private:
        LogChecker* mNext;
    };

    // Our implementation of HWC's abstract log class.
    // We supply this to HWC so that we can intercept (and parse) its log entries.
    class LogIntercept : public ::intel::ufo::hwc::AbstractLogWrite
    {
    private:
        ::intel::ufo::hwc::AbstractLogWrite* mRealLog;
        char* mInterceptedEntry;
        Hwcval::LogChecker *mChecker;

    public:
        // Implementations of AbstractLog functions
        virtual char* reserve(uint32_t maxSize);

        virtual void log(char* endPtr);

        // Control functions
        void Register( Hwcval::LogChecker* logChecker,
                       ::intel::ufo::hwc::validation::AbstractCompositionChecker* compositionChecker,
                       uint32_t compositionVersionsSupported);

        ::intel::ufo::hwc::AbstractLogWrite* GetRealLog();
    };

    inline ::intel::ufo::hwc::AbstractLogWrite* LogIntercept::GetRealLog()
    {
        return mRealLog;
    }

    typedef ::intel::ufo::hwc::AbstractLogWrite* (*SetLogValPtr) (::intel::ufo::hwc::AbstractLogWrite* logVal,
                                   ::intel::ufo::hwc::validation::AbstractCompositionChecker* checkComposition,
                                   uint32_t& versionSupportMask);
}


#endif // __Hwcval_LogIntercept_h__

