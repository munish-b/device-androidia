/****************************************************************************

Copyright (c) Intel Corporation (2014-15).

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

File Name:      HwcvalLogParser.cpp

Description:    Implements Log parsing functionality

Environment:

Notes:

****************************************************************************/

#include "HwcTestState.h"
#include "HwcTestKernel.h"

#if defined(HWCVAL_BUILD_HWCSERVICE_API) && ANDROID_VERSION<600
#include "re2/re2.h"
#include "re2/stringpiece.h"
#endif

#include "HwcvalLogParser.h"

#define CALL_PARSER_FUNC(name) { \
    if ((name)(pid, timestamp, str)) \
    {                \
      return true;   \
    } }

// Log validation
bool Hwcval::LogParser::DoParse(pid_t pid, int64_t timestamp, const char* str)
{
    // pid and timestamp parameters for future use.
    HWCVAL_UNUSED(pid);

    if (ParseKernel(pid, timestamp, str))
    {
        return true;
    }

#ifdef HWCVAL_BUILD_HWCSERVICE_API
    // See if the string has the HWC Service Api prefix
    if (strncmp("HwcService_", str, 11) == 0)
    {
        HWCCHECK(eCheckUnknownHWCAPICall);
        if (!ParseHWCServiceApi(pid, timestamp, str))
        {
            HWCERROR(eCheckUnknownHWCAPICall, "Log parser could not parse: %s", str);
        };

        return true;
    }
#endif

    return false;
}

bool Hwcval::LogParser::ParseKernel(pid_t pid, int64_t timestamp, const char* str)
{
    CALL_PARSER_FUNC(ParseBufferNotifications);
    CALL_PARSER_FUNC(ParseOptionSettings);
    CALL_PARSER_FUNC(ParseCompositionChoice);
    CALL_PARSER_FUNC(ParseRotationInProgress);

    return false;
}

#ifdef HWCVAL_BUILD_HWCSERVICE_API

bool Hwcval::LogParser::ParseHWCServiceApi(pid_t pid, int64_t timestamp, const char *str)
{
    CALL_PARSER_FUNC(ParseEnableEncryptedSessionEntry);
    CALL_PARSER_FUNC(ParseEnableEncryptedSessionExit);
    CALL_PARSER_FUNC(ParseDisableEncryptedSessionEntry);
    CALL_PARSER_FUNC(ParseDisableEncryptedSessionExit);
    CALL_PARSER_FUNC(ParseDisableAllEncryptedSessionsEntry);
    CALL_PARSER_FUNC(ParseDisableAllEncryptedSessionsExit);
    CALL_PARSER_FUNC(ParseDisplayModeGetAvailableModesEntry);
    CALL_PARSER_FUNC(ParseDisplayModeGetAvailableModesExit);
    CALL_PARSER_FUNC(ParseDisplayModeGetModeEntry);
    CALL_PARSER_FUNC(ParseDisplayModeGetModeExit);
    CALL_PARSER_FUNC(ParseDisplayModeSetModeEntry);
    CALL_PARSER_FUNC(ParseDisplayModeSetModeExit);
    CALL_PARSER_FUNC(ParseMDSUpdateVideoStateEntry);
    CALL_PARSER_FUNC(ParseMDSUpdateVideoStateExit);
    CALL_PARSER_FUNC(ParseMDSUpdateVideoFPSEntry);
    CALL_PARSER_FUNC(ParseMDSUpdateVideoFPSExit);
    CALL_PARSER_FUNC(ParseMDSUpdateInputStateEntry);
    CALL_PARSER_FUNC(ParseMDSUpdateInputStateExit);
    CALL_PARSER_FUNC(ParseSetOptimizationModeEntry);
    CALL_PARSER_FUNC(ParseSetOptimizationModeExit);
    CALL_PARSER_FUNC(ParseWidiSetSingleDisplayEntry);
    CALL_PARSER_FUNC(ParseWidiSetSingleDisplayExit);

    return false;
}

template <typename T, typename U>
bool Hwcval::LogParser::MatchRegex(const char *regex, const char *line,
    int32_t *num_fields_matched, T* match1_ptr, U* match2_ptr)
{
    bool result = false;

    if (num_fields_matched != nullptr)
    {
        *num_fields_matched = 0;
    }

#if ANDROID_VERSION >= 600
    // M-Dessert or greater - use C++11 Regexs
    std::regex hwcsRegex(regex);
    std::cmatch match;
    result = std::regex_search(line, match, hwcsRegex);
    if (!result)
    {
        return false; // No match
    }

    // We got a match - extract any captured fields
    HWCCHECK(eCheckLogParserError);
    if (match.empty() && (match1_ptr != nullptr || match2_ptr != nullptr))
    {
        // Programming error - the caller is expecting to capture fields, but
        // the regex match has not produced any.
        HWCERROR(eCheckLogParserError, "Expecting to extract fields, but regex match is empty!");
        return false;
    }

    // Extract the field(s)
    if ((match.size() >= 2) && ExtractField(match[1], match1_ptr) && num_fields_matched)
    {
        (*num_fields_matched)++;
    }

    if ((match.size() == 3) && ExtractField(match[2], match2_ptr) && num_fields_matched)
    {
        (*num_fields_matched)++;
    }
#else
    // This is an L-Dessert build - use RE2. Note, you have to call FullMatch
    // with the correct number of parameters (i.e. its using variable arguments.)
    if (!match1_ptr && !match2_ptr)
    {
        result = RE2::PartialMatch(line, regex);
    }
    else if (match1_ptr && !match2_ptr)
    {
        result = RE2::PartialMatch(line, regex, match1_ptr);
        if (result && num_fields_matched)
        {
            *num_fields_matched = 1;
        }
    }
    else if (match1_ptr && match2_ptr)
    {
        result = RE2::PartialMatch(line, regex, match1_ptr, match2_ptr);
        if (result && num_fields_matched)
        {
            *num_fields_matched = 2;
        }
    }
#endif

    return result;
}

#if defined(HWCVAL_BUILD_HWCSERVICE_API) && ANDROID_VERSION>=600
// Field extraction overloads
template <typename T>
bool Hwcval::LogParser::ExtractField(const std::cmatch::value_type field, T *field_ptr)
{
    if (field_ptr)
    {
        *field_ptr = std::stoi(field);
        return true;
    }

    return false;
}

bool Hwcval::LogParser::ExtractField(const std::cmatch::value_type field, char *field_ptr)
{
    if (field_ptr)
    {
        // Callers responsibiility to make sure there is enough memory available
        std::strcpy(field_ptr, field.str().c_str());
        return true;
    }

    return false;
}
#endif

// Common parsing functionality (i.e. patterns which match across multiple functions)
bool Hwcval::LogParser::ParseCommonExit(const char* str, const char* fn, android::status_t *ret)
{
    // Functions can either return 'OK' or 'ERROR' (plus a return value).
    std::string regex_ok("HwcService_" + std::string(fn) + " OK <--");
    if (MatchRegex(regex_ok.c_str(), str))
    {
       if (ret)
       {
         *ret = android::OK;
       }

       HWCLOGD_COND(eLogParse, "PARSED MATCHED %s - %s exit (return code: %d)",
         str, fn, android::OK);
       return true;
    }

    int32_t ret_val = 0, num_fields_matched = 0;

    std::string regex_error("HwcService_" + std::string(fn) + " ERROR (-?\\d+) <--");
    if (MatchRegex(regex_error.c_str(), str, &num_fields_matched, &ret_val))
    {
        if (ret)
        {
          *ret = ret_val;
        }

        HWCLOGD_COND(eLogParse, "PARSED MATCHED %s - %s exit (return code: %d)",
          str, fn, ret_val);
       return true;
    }

    return false;
}

// Individual parsing functionality (i.e. patterns which match single functions)
bool Hwcval::LogParser::ParseEnableEncryptedSessionEntry(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);

    int32_t num_fields_matched = 0, mParsedEncEnableInstance = 0;
    uint32_t mParsedEncEnableSession = 0; // Types match prints in HwcService.cpp

    if (MatchRegex("HwcService_Video_EnableEncryptedSession sessionID:(\\d+) instanceID:(\\d+) -->",
         str, &num_fields_matched, &mParsedEncEnableSession, &mParsedEncEnableInstance))
    {
        HWCCHECK(eCheckLogParserError);
        if (num_fields_matched == 2)
        {
            HWCLOGD_COND(eLogParse, "PARSED MATCHED %s - enabling encrypted State for session: %d "
                "instance %d", str, mParsedEncEnableSession, mParsedEncEnableInstance);
            HWCVAL_LOCK(_l, mTestKernel->GetMutex());
            mProtChecker.EnableEncryptedSession(mParsedEncEnableSession, timestamp);

            mParsedEncEnableStartTime = systemTime(SYSTEM_TIME_MONOTONIC);

            return true;
        }
        else
        {
            HWCERROR(eCheckLogParserError, "%s: Failed to extract two fields!", __func__);
        }
    }

    return false;
}

bool Hwcval::LogParser::ParseEnableEncryptedSessionExit(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    if (MatchRegex("HwcService_Video_EnableEncryptedSession .* <--", str))
    {
        HWCLOGD_COND(eLogParse, "PARSED MATCHED %s - enable encrypted session exit", str);
        int64_t ns = systemTime(SYSTEM_TIME_MONOTONIC) - mParsedEncEnableStartTime;

        HWCCHECK(eCheckProtEnableStall);
        if (ns > (15 * HWCVAL_MS_TO_NS))
        {
            HWCERROR(eCheckProtEnableStall, "Enabling encrypted session %d instance %d took %fms",
                mParsedEncEnableSession, mParsedEncEnableInstance, double(ns)/double(HWCVAL_MS_TO_NS));
        }

        return true;
    }

    return false;
}

bool Hwcval::LogParser::ParseDisableEncryptedSessionEntry(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    int32_t num_fields_matched = 0;

    if (MatchRegex("HwcService_Video_DisableEncryptedSession sessionID:(\\d+) -->",
         str, &num_fields_matched, &mParsedEncDisableSession))
    {
        HWCCHECK(eCheckLogParserError);
        if (num_fields_matched == 1)
        {
            HWCVAL_LOCK(_l, mTestKernel->GetMutex());
            mParsedEncDisableStartTime = timestamp;
            HWCLOGD_COND(eLogParse, "PARSED MATCHED %s - disabling encrypted state for "
                "session: %d - current time is: %f", str, mParsedEncDisableSession, double(mParsedEncDisableStartTime) / HWCVAL_SEC_TO_NS);

            mProtChecker.DisableEncryptedSessionEntry(mParsedEncDisableSession);

            return true;
        }
        else
        {
            HWCERROR(eCheckLogParserError, "%s: Failed to extract one field!", __func__);
        }
    }

    return false;
}

bool Hwcval::LogParser::ParseDisableEncryptedSessionExit(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);

    if (MatchRegex("HwcService_Video_DisableEncryptedSession .* <--", str))
    {
        HWCLOGD_COND(eLogParse, "PARSED MATCHED %s - disabling encrypted session exit", str);
        HWCVAL_LOCK(_l, mTestKernel->GetMutex());
        mProtChecker.DisableEncryptedSessionExit(mParsedEncDisableSession, mParsedEncDisableStartTime, timestamp);
        mTestKernel->CheckInvalidSessionsDisplayed();

        return true;
    }

    return false;
}

bool Hwcval::LogParser::ParseDisableAllEncryptedSessionsEntry(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    if (MatchRegex("HwcService_Video_DisableAllEncryptedSessions -->", str))
    {
        HWCLOGD_COND(eLogParse, "PARSED MATCHED %s - disable all encrypted session entry", str);
        HWCVAL_LOCK(_l, mTestKernel->GetMutex());
        mProtChecker.DisableAllEncryptedSessionsEntry();

        return true;
    }

    return false;
}

bool Hwcval::LogParser::ParseDisableAllEncryptedSessionsExit(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    if (MatchRegex("HwcService_Video_DisableAllEncryptedSessions .* <--", str))
    {
        HWCLOGD_COND(eLogParse, "PARSED MATCHED %s - disable all encrypted session exit", str);
        HWCVAL_LOCK(_l, mTestKernel->GetMutex());
        mProtChecker.DisableAllEncryptedSessionsExit();
        mTestKernel->CheckInvalidSessionsDisplayed();

        return true;
    }

    return false;
}

bool Hwcval::LogParser::ParseDisplayModeGetAvailableModesEntry(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    int32_t num_fields_matched = 0, display = -1;

    if (MatchRegex("HwcService_DisplayMode_GetAvailableModes D(\\d) -->",
         str, &num_fields_matched, &display))
    {
        HWCCHECK(eCheckLogParserError);
        if (num_fields_matched == 1)
        {
            HWCLOGD_COND(eLogParse, "PARSED MATCHED %s - got available modes for D%d",
                str, display);
            return true;
        }
        else
        {
            HWCERROR(eCheckLogParserError, "%s: Failed to extract one field!", __func__);
        }
    }

    return false;
}

bool Hwcval::LogParser::ParseDisplayModeGetAvailableModesExit(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    if (MatchRegex("HwcService_DisplayMode_GetAvailableModes .* <--", str))
    {
        HWCLOGD_COND(eLogParse, "PARSED MATCHED %s - exiting GetAvailableModes", str);
        return true;
    }

    return false;
}

bool Hwcval::LogParser::ParseDisplayModeGetModeEntry(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    int32_t num_fields_matched = 0, display = -1;

    if (MatchRegex("HwcService_DisplayMode_GetMode D(\\d) -->", str, &num_fields_matched, &display))
    {
        HWCCHECK(eCheckLogParserError);
        if (num_fields_matched == 1)
        {
            HWCLOGD_COND(eLogParse, "PARSED MATCHED %s - got mode for D%d", str, display);
            return true;
        }
        else
        {
            HWCERROR(eCheckLogParserError, "%s: Failed to extract one field!", __func__);
        }
    }

    return false;
}

bool Hwcval::LogParser::ParseDisplayModeGetModeExit(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    if (MatchRegex("HwcService_DisplayMode_GetMode .* <--", str))
    {
        return true;
    }

    return false;
}

bool Hwcval::LogParser::ParseDisplayModeSetModeEntry(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    int32_t num_fields_matched = 0, display = -1;
    std::string mode_str;

    if (MatchRegex("HwcService_DisplayMode_SetMode D(\\d) (.*) -->", str, &num_fields_matched, &display, &mode_str))
    {
        HWCCHECK(eCheckLogParserError);
        if (num_fields_matched == 2)
        {
            HWCLOGD_COND(eLogParse, "PARSED MATCHED %s - set mode for D%d: %s", str, display, mode_str.c_str());
            HWCVAL_LOCK(_l, mTestKernel->GetMutex());

            // Extract the width, height, refresh rate, flags and aspect ratio
            HWCCHECK(eCheckLogParserError);
            if (!MatchRegex("(\\d+)x(\\d+)", str, &num_fields_matched, &mSetModeWidth, &mSetModeHeight) ||
                !MatchRegex("@(\\d+)", str, &num_fields_matched, &mSetModeRefresh) ||
                !MatchRegex("F:(\\d+), A:(\\d+)", str, &num_fields_matched, &mSetModeFlags, &mSetModeAspectRatio))
            {
                HWCERROR(eCheckLogParserError, "%s: Failed to parse mode string!", __func__);
            }

            HWCLOGD_COND(eLogParse, "PARSED MATCHED %s - width %d height %d refresh %d flags %d aspect ratio %d",
                mode_str.c_str(), mSetModeWidth, mSetModeHeight, mSetModeRefresh, mSetModeFlags, mSetModeAspectRatio);

            HwcTestCrtc *crtc = mTestKernel->GetHwcTestCrtcByDisplayIx(display, true);
            if (crtc)
            {
                mSetModeDisplay = display;
                crtc->SetUserModeStart();
            }
            else
            {
                HWCLOGW("Can't set user mode for display %d as no CRTC defined", display);
            }

            return true;
        }
        else
        {
            HWCERROR(eCheckLogParserError, "%s: Failed to extract two fields!", __func__);
        }
    }

    return false;
}

bool Hwcval::LogParser::ParseDisplayModeSetModeExit(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    android::status_t ret_val = android::BAD_VALUE;

    if (ParseCommonExit(str, "DisplayMode_SetMode", &ret_val))
    {
        HWCVAL_LOCK(_l, mTestKernel->GetMutex());
        HWCLOGD_COND(eLogParse, "PARSED MATCHED %s - set mode exit (return code: %d)", str, ret_val);

        HwcTestCrtc *crtc = mTestKernel->GetHwcTestCrtcByDisplayIx(mSetModeDisplay, true);
        if (crtc)
        {
            crtc->SetUserModeFinish(ret_val, mSetModeWidth, mSetModeWidth, mSetModeRefresh,
                mSetModeFlags, mSetModeAspectRatio);
        }
        else
        {
            HWCLOGW("Can't set user mode finish for display %d as no CRTC defined", mSetModeDisplay);
        }

        return true;
    }

    return false;
}

bool Hwcval::LogParser::ParseMDSUpdateVideoStateEntry(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    int32_t prepared = 0, num_fields_matched = 0;
    int64_t session = 0;

    if (MatchRegex("HwcService_MDS_UpdateVideoState session:(\\d+), prepared:(\\d+) -->",
         str, &num_fields_matched, &session, &prepared))
    {
        mTestKernel->UpdateVideoState(session, prepared);

        HWCLOGD_COND(eLogParse, "PARSED MATCHED %s - setting MDS Video State for session: %" PRId64
            " (prepared: %d)", str, session, prepared);
        return true;
    }

    return false;
}

bool Hwcval::LogParser::ParseMDSUpdateVideoStateExit(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    return ParseCommonExit(str, "MDS_UpdateVideoState");
}

bool Hwcval::LogParser::ParseMDSUpdateVideoFPSEntry(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    int32_t num_fields_matched = 0, fps = 0;
    int64_t session = 0;

    if (MatchRegex("HwcService_MDS_UpdateVideoFPS session:(\\d+), fps:(\\d+) -->",
         str, &num_fields_matched, &session, &fps))
    {
        mTestKernel->UpdateVideoFPS(session, fps);

        HWCLOGD_COND(eLogParse, "PARSED MATCHED %s - setting MDS Video FPS for session: %d "
            "to %d", str, session, fps);
        return true;
    }

    return false;
}

bool Hwcval::LogParser::ParseMDSUpdateVideoFPSExit(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    return ParseCommonExit(str, "MDS_UpdateVideoFPS");
}

bool Hwcval::LogParser::ParseMDSUpdateInputStateEntry(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    int32_t num_fields_matched = 0;
    int64_t state = 0;

    if (MatchRegex("HwcService_MDS_UpdateInputState (\\d) -->", str, &num_fields_matched, &state))
    {
        mTestKernel->UpdateInputState(state);

        HWCLOGD_COND(eLogParse, "PARSED MATCHED %s - setting MDS Input State to %s",
            str, state ? "true" : "false");
        return true;
    }

    return false;
}

bool Hwcval::LogParser::ParseMDSUpdateInputStateExit(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    return ParseCommonExit(str, "MDS_UpdateInputState");
}

bool Hwcval::LogParser::ParseSetOptimizationModeEntry(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    int32_t num_fields_matched = 0, opt_mode = 0;

    if (MatchRegex("HwcService_Video_SetOptimizationMode (\\d+)", str, &num_fields_matched, &opt_mode))
    {
        mParsedOptimizationMode =
            static_cast<intel::ufo::hwc::services::IVideoControl::EOptimizationMode>(opt_mode);

        mTestKernel->CheckSetOptimizationModeEnter(mParsedOptimizationMode);

        HWCLOGD_COND(eLogParse, "PARSED MATCHED %s - set video optimisation mode: %d ", str, mParsedOptimizationMode);
        return true;
    }

    return false;
}

bool Hwcval::LogParser::ParseSetOptimizationModeExit(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    int32_t num_fields_matched = 0, ret_val = 0;

    if (MatchRegex("HwcService_Video_SetOptimizationMode OK <--", str))
    {
        mTestKernel->CheckSetOptimizationModeExit(android::OK, mParsedOptimizationMode);

        HWCLOGD_COND(eLogParse, "PARSED MATCHED %s - set video optimisation mode exit (OK)",
            str);
        return true;
    }
    else if (MatchRegex("HwcService_Video_SetOptimizationMode ERROR (\\d+) <--", str, &num_fields_matched, &ret_val))
    {
        if (num_fields_matched == 1)
        {
            mTestKernel->CheckSetOptimizationModeExit(ret_val, mParsedOptimizationMode);

            HWCLOGD_COND(eLogParse, "PARSED MATCHED %s - set video optimisation mode exit (return code: %d)",
                str, ret_val);
            return true;
        }
        else
        {
            HWCERROR(eCheckLogParserError, "%s: Failed to extract return value", __func__);
        }
    }

    return false;
}

bool Hwcval::LogParser::ParseWidiSetSingleDisplayEntry(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    int32_t num_fields_matched = 0, mode = 0;

    if (MatchRegex("HwcService_Widi_SetSingleDisplay (\\d+) -->", str, &num_fields_matched, &mode))
    {
        HWCLOGD_COND(eLogParse, "PARSED MATCHED %s - setting Widi single display mode to: %d", str, mode);

        // Cache the mode for ParseWidiSetSingleDisplayExit
        mTestKernel->SetWirelessDockingMode(mode);

        return true;
    }

    return false;
}

bool Hwcval::LogParser::ParseWidiSetSingleDisplayExit(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    return ParseCommonExit(str, "Widi_SetSingleDisplay");
}
#endif

bool Hwcval::LogParser::ParseBufferNotifications(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    const char* p = strafter(str, "BufferManager: Notification free buffer handle ");

    if (p == 0)
    {
        return false;
    }

    uintptr_t h = atoptrinc(p);

    if (h != 0)
    {
        buffer_handle_t handle = (buffer_handle_t) h;

        HWCLOGD_COND(eLogParse, "PARSED MATCHED %s Freeing %p", str, handle);
        mTestKernel->GetWorkQueue().Push(new Hwcval::Work::BufferFreeItem(handle));
    }

    return true;
}

bool Hwcval::LogParser::ParseOption(const char*& p)
{
    skipws(p);
    android::String8 optionName = getWord(p);
    skipws(p);

    if (*p++ != ':')
    {
        return false;
    }

    if (*p++ != ' ')
    {
        return false;
    }

    // The value string is not quoted, so we have to use some logic to work out where it ends.
    const char* pWords[256];
    android::String8 words[256];
    const char* endOfValue = 0;

    for (uint32_t nw=0; nw<256 && *p && (*p != '\n'); ++nw)
    {
        pWords[nw] = p;
        words[nw] = getWord(p);
        skipws(p);

        if ((nw > 3) && (words[nw] == "Changable") && (words[nw-1] == "Int" || words[nw-1] == "Str"))
        {
            for (const char* p2 = pWords[nw-2]; p2 != pWords[nw-3]; --p2)
            {
                if (*p2 == '(')
                {
                    endOfValue = p2;
                    break;
                }
            }
        }
    }

    android::String8 value;
    if (endOfValue)
    {
        HWCLOGV_COND(eLogOptionParse, "%s pWords[0] %p %s", optionName.string(), pWords[0], pWords[0]);
        HWCLOGV_COND(eLogOptionParse, "endOfValue %p %s", endOfValue, endOfValue);
        value = android::String8(pWords[0], endOfValue - pWords[0]);
        HWCLOGV_COND(eLogOptionParse, "value: %s", value.string());
    }
    else
    {
        value = words[0];
        HWCLOGV_COND(eLogOptionParse, "ParseOption: %s = %s",optionName.string(), value.string());
    }

    mTestKernel->SetHwcOption(optionName, value);
    return true;
}

bool Hwcval::LogParser::ParseOptionSettings(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    const char* p = str;
    if (strncmpinc(p, "Option ") == 0)
    {
        if (strncmpinc(p, "Values:") == 0)
        {
            while (*p++ == '\n')
            {
                if (!ParseOption(p))
                {
                    return false;
                }

                while (*p && (*p != '\n'))
                {
                    p++;
                }
            }
        }
        else if ((strncmpinc(p, "Default ") == 0) ||
            (strncmpinc(p, "Forced ") == 0))
        {
            return ParseOption(p);
        }
    }

    return false;
}

static Hwcval::Statistics::Counter numSfFallbackCompositions("sf_fallback_compositions");
static Hwcval::Statistics::Counter numTwoStageFallbackCompositions("two_stage_fallback_compositions");
static Hwcval::Statistics::Counter numLowlossCompositions("lowloss_compositions");

bool Hwcval::LogParser::ParseCompositionChoice(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    // Fallbacks are highly undesirable, ultimately we may log these as errors.
    const char* p = strafter(str, "fallbackToSurfaceFlinger!");

    if (p == 0)
    {
        p = str;
        if (strncmpinc(p, "TwoStageFallbackComposer") == 0)
        {
            ++numTwoStageFallbackCompositions;
            return true;
        }
        else if (strncmpinc(p, "LowlossComposer") == 0)
        {
            ++numLowlossCompositions;
            return true;
        }

        return false;
    }

    p = str;
    if (*p != 'D')
    {
        return false;
    }

    //uint32_t d = atoi(p);

    ++numSfFallbackCompositions;

    // HWCCHECK count will be set at end of test when we know how many compositions there were
    HWCERROR(eCheckSfFallback, "Not required, TwoStageFallback should be used");
    return true;
}

bool Hwcval::LogParser::ParseRotationInProgress(pid_t pid, int64_t timestamp, const char* str)
{
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    const char* p = str;
    if (strncmpinc(p, "Rotation in progress") != 0)
    {
        return false;
    }

    const char* p2 = strafter(p, "FrameKeepCnt: ");
    if (p2 == 0) return false;

    uint32_t f = atoi(p2);

    p2 = strafter(p, "SnapshotLayerHandle: ");
    if (p2 == 0) return false;
    uintptr_t h = atoptrinc(p2);

    if (h == 0) return false;

    buffer_handle_t handle = (buffer_handle_t) h;
    HWCLOGD_COND(eLogParse, "PARSED MATCHED: Rotation in progress FrameKeepCnt %d handle %p", f, handle);

    mTestKernel->SetSnapshot(handle, f);

    return true;
}
