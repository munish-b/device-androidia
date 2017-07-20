/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include "HwcTestLog.h"
#include <utils/String8.h>
#include "HwcTestConfig.h"
#include "HwcTestUtil.h"
#ifndef HWCVAL_IN_TEST
#include "HwcTestState.h"
#endif

#ifdef HWCVAL_ABSTRACTLOG_EXISTS
#include "AbstractLog.h"
#include "HwcvalLogIntercept.h"

Hwcval::SetLogValPtr pfHwcLogSetLogVal = 0;
Hwcval::LogIntercept gLogIntercept;

// Print HWCVAL logging to standard output
//#define HWCVAL_PRINT_LOG
// Print HWC logging to standard output
//#define HWCVAL_PRINT_HWC_LOG

//
// Called by the logging entry before the log entry is constructed.
// Returns a pointer the location where the entry will be written.
char* Hwcval::LogIntercept::reserve(uint32_t maxSize)
{
    if (mRealLog)
    {
        mInterceptedEntry = mRealLog->reserve(maxSize);
        return mInterceptedEntry;
    }
    else
    {
        return 0;
    }
}

//
// Called by the logging framework after the log entry has been written to memory.
// endPtr is the byte after the end of the entry.
//
// Our job here is not to modify the entry but to unpack it and pass it to our parser.
//
void Hwcval::LogIntercept::log(char* endPtr)
{
    if (mRealLog)
    {
        // Copy the intercepted entry
        char entry[2 * HWCLOG_STRING_RESERVATION_SIZE];
        pid_t pid;
        int64_t timestamp;
        const char* interceptedEntry = mRealLog->unpack(mInterceptedEntry, pid, timestamp);

        if (endPtr > interceptedEntry)
        {
            size_t size = endPtr - interceptedEntry;
            size = min(size, sizeof(entry)-1);

            strncpy(entry, interceptedEntry, size);
            entry[size] = '\0';

            // Pass the log entry on to the log, BEFORE we process it
            // since our processing is itself likely to lead to log entries
            // and otherwise this would deadlock.
            mRealLog->log(endPtr);

            // Filter and process the log entry
            if (mChecker)
            {
#ifdef HWCVAL_PRINT_HWC_LOG
                printf("TID:%d %s\n", pid, entry);
#endif
                mChecker->Parse(pid, timestamp, entry);
            }
        }
    }
}

//
// Register our log interception with HWC.
//
void Hwcval::LogIntercept::Register(  Hwcval::LogChecker* logChecker,
                                    intel::ufo::hwc::validation::AbstractCompositionChecker* compositionChecker,
                                    uint32_t compositionVersionsSupported)
{
    if (pfHwcLogSetLogVal)
    {
        mChecker = logChecker;
        uint32_t hwcSupportedVersionMask = 0;

        // Pass our log interceptor and our composition checker into HWC
        // HWC gives us back a pointer to the "real" logger so
        //    (a) we can use it to write log entries of our own
        //    (b) we can reset back to the default state during shutdown if necessary.
        //
        mRealLog = (pfHwcLogSetLogVal)( this,
                                        compositionChecker,
                                        hwcSupportedVersionMask);

        if ((hwcSupportedVersionMask & compositionVersionsSupported) == 0)
        {
            (pfHwcLogSetLogVal)(this, 0, hwcSupportedVersionMask);
            HWCERROR(eCheckInternalError, "AbstractCompositionChecker incompatible between HWC and validation.");
            HWCLOGE("  - Composition interception disabled, checks will fail.");
        }
        else
        {
            // 2nd call is to get the HWC option values dumped into the log.
            // First time this doesn't happen because the log redirection is in progress.
            (pfHwcLogSetLogVal)( this,
                                compositionChecker,
                                hwcSupportedVersionMask);
        }
    }
}


#endif // HWCVAL_ABSTRACTLOG_EXISTS


// General HWC validation message logger
// Normal function - log to HWC log viewer if available (i.e. normally if in SF process), log to Android log otherwise.
// Define HWCVAL_LOG_ANDROID to ensure that BOTH logs are used whenever possible.
// Define HWCVAL_LOG_ANDROIDONLY to ensure that only Android log is used.
int HwcValLogVerbose(int priority, const char* context, int line, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    android::String8 logStr=android::String8::formatV(fmt, args);

    //android::String8 fmt2 = android::String8::format("%s(%d) ",context, line);
    android::String8 fmt2("%s(%d) %s");
    HwcValLog(priority, fmt2.string(), context, line, logStr.string());

    va_end(args);
    return priority;
}

static const char* sPriorities="A-VDIWEFS"; // Matches Android priorities.
    // except we have Always instead of Unknown.

// Log to the HWC logger at the stated priority level
int HwcValLog(int priority, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    HwcValLogVA(priority, fmt, args);

    va_end(args);
    return priority;
}

// Log varargs to the HWC logger
int HwcValLogVA(int priority, const char* fmt, va_list& args)
{
// Get a pointer to HWC's real logger function.
// Note that HWC validation intercepts everything that HWC itself writes to the log for parsing
// before forwarding it to the real log.
//
// These are HWC validation messages, so they go straight to the "real log" or else we would
// have an infinite loop.
#ifndef HWCVAL_LOG_ANDROIDONLY
#ifdef HWCVAL_ABSTRACTLOG_EXISTS
// Create macros for calling the real logger
#define _HWCLOG gLogIntercept.GetRealLog()->add
#define _HWCLOGV  gLogIntercept.GetRealLog()->addV
    if (gLogIntercept.GetRealLog())
#else
// This is the old version, to retain compatibility with any HWC that does not support log interception.
#define _HWCLOG (*pLog)
    HwcTestState::HwcLogAddPtr pLog = HwcTestState::getInstance()->GetHwcLogFunc();

    if (pLog)
#endif
    {
        // We have obtained a pointer to the logger.
        // Construct the prefix to the log message.
        char prefix[10];
        strcpy(prefix, "HWCVAL:  ");
        if (priority>0 && priority < (int)strlen(sPriorities))
        {
            prefix[7] = sPriorities[priority];
        }

#if __x86_64__
        //
        // If HWCVAL_LOG_HWC_ANDROID is defined, OR if the priority is "ALWAYS", "ERROR" or "FATAL",
        // then DUPLICATE the log entry to LOGCAT.
        //
#if !defined(HWCVAL_LOG_HWC_ANDROID)
        // HWCLOGA, HWCLOGE and HWCLOGF always go both to HWCLOG and to logcat.
        if ((priority == ANDROID_LOG_UNKNOWN) || (priority >= ANDROID_LOG_ERROR))
#endif
        {
#ifdef HWCVAL_ABSTRACTLOG_EXISTS
            android::String8 fmt3(prefix);
            fmt3 += fmt;
            const char* ptr = _HWCLOGV(fmt3, args);
            if (ptr && (strlen(ptr) > 9))
            {
                // Log to logcat
                LOG_PRI((priority == ANDROID_LOG_UNKNOWN) ? ANDROID_LOG_INFO : priority, "HWCVAL", "%s", ptr+9);

// Debugging option to duplicate the log entry to standard out.
#ifdef HWCVAL_PRINT_LOG
                printf("TID:%d %s\n", gettid(), ptr+9);
#endif
            }
#else
            android::String8 formattedString = android::String8::formatV(fmt, args);
            android::String8 hwclogString(prefix);
            hwclogString += formattedString;

            // Send the combined log string to HwcLogViewer
            _HWCLOG (hwclogString.string());

// Debugging option to duplicate the log entry to standard out.
#ifdef HWCVAL_PRINT_LOG
            printf("TID: %d%s\n", gettid(), hwclogString.string());
#endif

            // Send the combined log string to Android
            LOG_PRI(priority, "HWCVAL", "%s", formattedString.string());
#endif
            va_end(args);

            // ** Early return for the "log to hwclog and logcat" case.
            return priority;
        }
#endif // __x86_64__

#if !__x86_64__ || !defined(HWCVAL_LOG_HWC_ANDROID)

        // Prefix the format buffer with the string "HWCVAL:x"
        #define FMT_BUF_SIZE 1024
        char fmt3[FMT_BUF_SIZE];
        strcpy(fmt3, prefix);
        strncat(fmt3, fmt, FMT_BUF_SIZE-10);

#ifdef HWCVAL_PRINT_LOG
        // For DEBUG only: create the log entry as a string8 and output it to logcat and standard out.
        android::String8 hwcLogStr = android::String8::formatV(fmt3, args);
        _HWCLOG(hwcLogStr.string());
        printf("TID:%d %s\n", gettid(), hwcLogStr.string());
#else
#ifdef HWCVAL_ABSTRACTLOG_EXISTS
        // Send the combined log string to HwcLogViewer
        //
        // *** MAIN NORMAL LOGGING TO HWCLOG ***
        // This is the common and most efficient form. We avoid the creation of dynamic strings entirely
        // since the logger itself will sprint directly into its circular buffer.
        _HWCLOGV(fmt3, args);
#else
        // Legacy form for HWC without log interception.
        _HWCLOG (android::String8::formatV(fmt3, args).string());
#endif
#endif

#endif
    }

// This last part is fallback.
// If we can't obtain a pointer to the logger (generally because this is very early on and it hasn't been
// created yet) then log to logcat instead.
#ifndef HWCVAL_LOG_HWC_ANDROID
#ifdef HWCVAL_ABSTRACTLOG_EXISTS
    if ((gLogIntercept.GetRealLog() == 0) || (priority == ANDROID_LOG_UNKNOWN))
#else
    if ((pLog == 0) || (priority == ANDROID_LOG_UNKNOWN))
#endif
#endif // HWCVAL_LOG_HWC_ANDROID
#endif // !HWCVAL_LOG_ANDROIDONLY
    {
        LOG_PRI_VA(priority, "HWCVAL", fmt, args);
    }

    return priority;
}

//
// Log an ERROR
// This has two lines:
// 1. HWCVAL:E, followed by the message obtained by looking up the enum "check" in HwcTestConfig::mCheckDescriptions
//    (this is populated by the data in HwcTestCheckList.h).
// 2. The formatted string constructed from fmt and the other parameters, which is indented by "  -- ".
//
int HwcValError(HwcTestCheckType check, HwcTestConfig* config, HwcTestResult* result, const char* fmt, ...)
{
    if (!config)
    {
        return ANDROID_LOG_ERROR;
    }

    result->SetFail(check);
    int priority = config->mCheckConfigs[check].priority;
    android::String8 msg(HwcTestConfig::mCheckDescriptions[check]);
    msg += "\n  -- ";

    va_list args;
    va_start(args, fmt);
    msg += fmt;
    HwcValLogVA(priority, msg.string(), args);
    va_end(args);

    if (priority == ANDROID_LOG_FATAL)
    {
        Hwcval::ValCallbacks::DoExit();
    }

    return priority;
}

