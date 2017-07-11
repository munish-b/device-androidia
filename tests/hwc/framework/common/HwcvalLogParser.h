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

File Name:      HwcvalLogParser.h

Description:    Hwc log parsing functionality.

Environment:

Notes:

****************************************************************************/

#ifndef __Hwcval_LogParser_h__
#define __Hwcval_LogParser_h__

#include "HwcvalLogIntercept.h"
#include "IVideoControl.h"

#if defined(HWCVAL_BUILD_HWCSERVICE_API) && ANDROID_VERSION>=600
#include <regex>
#endif

class HwcTestKernel;
class HwcTestProtectionChecker;

namespace Hwcval
{
    class LogParser : public Hwcval::LogChecker
    {
    private:
        // Pointers and references to internal objects
        HwcTestKernel* mTestKernel = nullptr;
        HwcTestProtectionChecker& mProtChecker;

        int64_t mParsedEncEnableStartTime = 0;
        int64_t mParsedEncDisableStartTime = 0;
        uint32_t mParsedEncDisableSession = 0;
        uint32_t mParsedEncEnableSession = 0;
        uint32_t mParsedEncEnableInstance = 0;
        uint32_t mSetModeDisplay = 0;
        uint32_t mSetModeWidth = 0;
        uint32_t mSetModeHeight = 0;
        uint32_t mSetModeRefresh = 0;
        uint32_t mSetModeFlags = 0;
        uint32_t mSetModeAspectRatio = 0;
        ::intel::ufo::hwc::services::IVideoControl::EOptimizationMode mParsedOptimizationMode;

        // Parser functionaity
        bool ParseHWCServiceApi(pid_t pid, int64_t timestamp, const char *str);

        // Common functionality

        // Top-level function to match a regex to a string. This uses C++11 regexs
        // on M-Dessert and above, and RE2 on L-Dessert.
        template <typename T = void, typename U = T>
        bool MatchRegex(const char *regex, const char *line, int32_t *num_fields_matched = nullptr,
            T* match1_ptr = nullptr, U* match2_ptr = nullptr);

#if defined(HWCVAL_BUILD_HWCSERVICE_API) && ANDROID_VERSION>=600
        // Overloads for field extraction
        template <typename T>
        bool ExtractField(const std::cmatch::value_type field, T *field_ptr);
        bool ExtractField(const std::cmatch::value_type field, char *field_ptr);

        // Default overload (for when no match parameters are specified to MatchRegex)
        bool ExtractField(const std::cmatch::value_type field, void* dummy)
        {
            HWCVAL_UNUSED(field);
            HWCVAL_UNUSED(dummy);

            return false;
        };
#endif

        bool ParseCommonExit(const char* str, const char* fn, android::status_t *ret = nullptr);

        // Functionality for individual functions
        bool ParseEnableEncryptedSessionEntry(pid_t pid, int64_t timestamp, const char* str);
        bool ParseEnableEncryptedSessionExit(pid_t pid, int64_t timestamp, const char* str);
        bool ParseDisableEncryptedSessionEntry(pid_t pid, int64_t timestamp, const char* str);
        bool ParseDisableEncryptedSessionExit(pid_t pid, int64_t timestamp, const char* str);
        bool ParseDisableAllEncryptedSessionsEntry(pid_t pid, int64_t timestamp, const char* str);
        bool ParseDisableAllEncryptedSessionsExit(pid_t pid, int64_t timestamp, const char* str);
        bool ParseDisplayModeGetAvailableModesEntry(pid_t pid, int64_t timestamp, const char* str);
        bool ParseDisplayModeGetAvailableModesExit(pid_t pid, int64_t timestamp, const char* str);
        bool ParseDisplayModeGetModeEntry(pid_t pid, int64_t timestamp, const char* str);
        bool ParseDisplayModeGetModeExit(pid_t pid, int64_t timestamp, const char* str);
        bool ParseDisplayModeSetModeEntry(pid_t pid, int64_t timestamp, const char* str);
        bool ParseDisplayModeSetModeExit(pid_t pid, int64_t timestamp, const char* str);
        bool ParseMDSUpdateVideoStateEntry(pid_t pid, int64_t timestamp, const char* str);
        bool ParseMDSUpdateVideoStateExit(pid_t pid, int64_t timestamp, const char* str);
        bool ParseMDSUpdateVideoFPSEntry(pid_t pid, int64_t timestamp, const char* str);
        bool ParseMDSUpdateVideoFPSExit(pid_t pid, int64_t timestamp, const char* str);
        bool ParseMDSUpdateInputStateEntry(pid_t pid, int64_t timestamp, const char* str);
        bool ParseMDSUpdateInputStateExit(pid_t pid, int64_t timestamp, const char* str);
        bool ParseSetOptimizationModeEntry(pid_t pid, int64_t timestamp, const char* str);
        bool ParseSetOptimizationModeExit(pid_t pid, int64_t timestamp, const char* str);
        bool ParseWidiSetSingleDisplayEntry(pid_t pid, int64_t timestamp, const char* str);
        bool ParseWidiSetSingleDisplayExit(pid_t pid, int64_t timestamp, const char* str);

        bool ParseKernel(pid_t pid, int64_t timestamp, const char* str);
        bool ParseBufferNotifications(pid_t pid, int64_t timestamp, const char* str);
        bool ParseOption(const char*& p);
        bool ParseOptionSettings(pid_t pid, int64_t timestamp, const char* str);
        bool ParseCompositionChoice(pid_t pid, int64_t timestamp, const char* str);
        bool ParseRotationInProgress(pid_t pid, int64_t timestamp, const char* str);

    public:

        // Constructor
        LogParser(HwcTestKernel *pKernel, HwcTestProtectionChecker& pChecker)
          : mTestKernel(pKernel), mProtChecker(pChecker) {};

        // Log parser entry point
        virtual bool DoParse(pid_t pid, int64_t timestamp, const char* str);
    };
}

#endif // __Hwcval_LogParser_h__
