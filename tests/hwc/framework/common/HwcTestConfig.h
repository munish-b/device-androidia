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

File Name:      HwcTestConfig.h

Description:    Configuration and status structures.

Environment:

Notes:          This hwc_shim.h/c are used in both c and cpp if the drm shim
                is rewritten to be cpp this can become a cpp class.

****************************************************************************/


#ifndef __HwcTestConfig_h__
#define __HwcTestConfig_h__

#include "HwcTestDefs.h"
#include <limits.h>
#include <cutils/log.h>

// Avoid pulling all includes in here
namespace android
{
    class Parcel;
}

enum HwcTestCategoryType
{
    eCatTest,       // Test-internal errors
    eCatSf,         // Errors attributed to surface flinger
    eCatDisplays,   // Errors attributed to display subsystem
    eCatBuffers,    // Errors in buffer management: could be gralloc, HWC or test problem
    eCatHwc,        // Errors detected in Hardware Composer
    eCatHwcDisplay, // Errors detected in Hardware Composer use of DRM or ADF
    eCatIvp,        // Errors detected in Hardware Composer use of iVP
    eCatWidi,       // Errors detected in Hardware Composer use of Widi
    eCatUX,         // User Experience
    eCatDbg,        // Enables of specific types of debug logs. Not checks.
    eCatOpt,        // Enables of specific options
    eCatPriWarn,    // Priority warning - not an error, but user should be told
    eCatStickyTest  // Sticky test failure. Will be reported on every test run.
};

enum HwcTestComponentType
{
    eComponentMin = 0,
    // ALWAYS ensure that matching strings are provided in HwcTestConfig::mComponentNames.
    eComponentNone = eComponentMin,
    eComponentTest,
    eComponentHWC,
    eComponentBuffers,
    eComponentDisplays,
    eComponentIVP,
    eComponentSF,
    // Add in new values before this.
    eComponentMax
};

#define DECLARE_CHECK(enumId, component, defaultPriority, description, category) enumId,
enum HwcTestCheckType
{
#include "HwcTestCheckList.h"

    eHwcTestNumChecks
};
#undef DECLARE_CHECK

union HwcCheckConfig
{
    struct
    {
        bool enable:                1;
        bool forceDisable:          1;
        bool causesTestFail:        1;
        unsigned int  priority:     4;      // Android priority level for this check
        unsigned int  errorLevel:   6;      // Error count at which check will be escalated to Error, or 0 if not required
        unsigned int  category:     4;      // Enable/disable category for the check
    };

    uint32_t value;

    HwcCheckConfig()
      : enable(false), forceDisable(false), causesTestFail(true), priority(0), errorLevel(0), category(0)
    {
    }
};


class HwcTestConfig
{
    // Data is all public here because this is really a struct with some serialisation methods
public:
    /// Minimum priority to appear in log (NOTE: Android use an int).
    int mMinLogPriority;

    /// Global enable for all checks
    bool mGlobalEnable;

    /// Global buffer monitor enable - once disabled, can't be re-enabled without restarting SF
    bool mBufferMonitorEnable;

    /// Display CRC check delay
    uint32_t mDisplayCRCCheckDelay;

    /// Test can configure what expectations it has of HWC mode
    enum PanelModeType
    {
        eDontCare = 0,
        eOff,
        eOn
    };
    PanelModeType mModeExpect;
    PanelModeType mStableModeExpect;

    /// Configuration of each check
    HwcCheckConfig mCheckConfigs[eHwcTestNumChecks];

    /// Name of each check
    static const char* mCheckNames[];

    /// Access to the name
    static const char* GetName(uint32_t check);

    /// Description of each check
    static const char* mCheckDescriptions[];

    /// Access to the description
    static const char* GetDescription(uint32_t check);

    /// Component tested by each check
    static HwcTestComponentType mCheckComponents[];

    /// Name of each component
    static const char* mComponentNames[];

    bool mComponentEnabled[eComponentMax];

    /// Access to component tested by each check
    static HwcTestComponentType GetComponent(uint32_t check);
    static const char* GetComponentName(uint32_t check);
    void SetComponentEnabled(uint32_t component, bool enabled, bool causesTestFail);
    bool IsComponentEnabled(uint32_t component);
    const char* GetComponentEnableStr(uint32_t component);

    // Convert string to a check tyoe
    HwcTestCheckType CheckFromName(const char* checkName);

    /// Constructor
    HwcTestConfig();

    // Perform standard initialisation
    void Initialise(bool valHwc, bool valDisplays, bool valBufferAllocation, bool valIvp, bool valSf,
                    bool valHwcComposition, bool valIvpComposition);

    // Serialisation
    bool WriteToParcel(android::Parcel& parcel) const;
    bool ReadFromParcel(const android::Parcel& parcel);

    /// Turn off all checks
    void DisableAllChecks();

    /// Enable a check
    void SetCheck(HwcTestCheckType check, bool enable=true, bool causesTestFail = true);

    // Set display mode expectation
    void SetModeExpect(PanelModeType modeExpect);

    /// Get display mode expectation
    PanelModeType GetModeExpect();
    PanelModeType GetStableModeExpect();

    /// Is check enabled?
    bool IsEnabled(HwcTestCheckType check);

    /// Is log level enabled?
    bool IsLevelEnabled(int priority);

    static const char* Str(PanelModeType panelMode);
};

inline const char* HwcTestConfig::GetDescription(uint32_t check)
{
    return mCheckDescriptions[check];
}

inline const char* HwcTestConfig::GetName(uint32_t check)
{
    return mCheckNames[check];
}

inline HwcTestComponentType HwcTestConfig::GetComponent(uint32_t check)
{
    return mCheckComponents[check];
}

inline const char* HwcTestConfig::GetComponentName(uint32_t check)
{
    return mComponentNames[GetComponent(check)];
}

inline void HwcTestConfig::SetModeExpect(HwcTestConfig::PanelModeType modeExpect)
{
    mModeExpect = modeExpect;
    mStableModeExpect = eDontCare;
}

inline HwcTestConfig::PanelModeType HwcTestConfig::GetModeExpect()
{
    return mModeExpect;
}

inline HwcTestConfig::PanelModeType HwcTestConfig::GetStableModeExpect()
{
    PanelModeType result = mStableModeExpect;
    mStableModeExpect = mModeExpect;
    return result;
}

inline bool HwcTestConfig::IsEnabled(HwcTestCheckType check)
{
    return mCheckConfigs[check].enable && mGlobalEnable;
}

inline bool HwcTestConfig::IsLevelEnabled(int priority)
{
    return ((priority >= mMinLogPriority) || (priority == ANDROID_LOG_UNKNOWN));
}

class HwcTestResult
{
public:
    // Persisted by the binder...
    struct PerDisplay
    {
        uint32_t mMaxConsecutiveDroppedFrameCount;
        uint32_t mDroppedFrameCount;
        uint32_t mFrameCount;
    };

    uint32_t mCheckFailCount[eHwcTestNumChecks];
    uint32_t mCheckEvalCount[eHwcTestNumChecks];

    PerDisplay mPerDisplay[HWCVAL_MAX_CRTCS];

    uint32_t mHwcCompValSkipped;
    uint32_t mHwcCompValCount;

    uint32_t mSfCompValSkipped;
    uint32_t mSfCompValCount;

    // Time results were reset
    int64_t mStartTime;
    int64_t mEndTime;

    // Not persisted by the binder...
    uint32_t mFinalPriority[eHwcTestNumChecks];
    bool mCausesTestFail[eHwcTestNumChecks];

    /// Constructor
    HwcTestResult();

    // Serialisation
    bool WriteToParcel(android::Parcel& parcel) const;
    bool ReadFromParcel(const android::Parcel& parcel);

    /// Combination
    HwcTestResult& operator+=(HwcTestResult& rhs);

    /// Reset all failure and evaluation counts to 0, except where sticky
    void Reset(HwcTestConfig* config = 0);

    /// End timestamp
    void SetEndTime();

    /// Set start and end timestamps
    void SetStartEndTime(int64_t startTime, int64_t endTime);

    /// Increment an evaluation count
    void IncEval(HwcTestCheckType check);
    void AddEval(HwcTestCheckType check, uint32_t additional);
    uint32_t GetEvalCount(HwcTestCheckType check);

    /// Increment a failure count
    void SetFail(HwcTestCheckType check, uint32_t add=1);

    /// Increment a failure count & report error
    int ReportE(HwcTestCheckType check, HwcTestConfig* config);

    // Copy priorities from config
    void CopyPriorities(HwcTestConfig& config);

    /// Set final check priority
    void SetFinalPriority(HwcTestCheckType check, int priority);
    /// conditionally to reducedPriority if failure count <= maxNormCount
    void ConditionalDropPriority(HwcTestCheckType check, uint32_t maxNormCount, int reducedPriority);
    /// conditionally back to config priority if failure count > maxNormCount
    void ConditionalRevertPriority(HwcTestConfig& config, HwcTestCheckType check, uint32_t maxNormCount);

    /// Combine failures bearing in mind severity of each
    bool IsGlobalFail();

    /// Log the results to standard out
    void Log(HwcTestConfig& config, const char* testName, bool brief);

    /// Log pass/fail only to standard out
    void LogTestPassFail(const char* testName);
};

inline void HwcTestResult::IncEval(HwcTestCheckType check)
{
    ++mCheckEvalCount[check];
}

inline void HwcTestResult::AddEval(HwcTestCheckType check, uint32_t additional)
{
    mCheckEvalCount[check] += additional;
}

inline uint32_t HwcTestResult::GetEvalCount(HwcTestCheckType check)
{
    return mCheckEvalCount[check];
}

inline void HwcTestResult::SetFail(HwcTestCheckType check, uint32_t add)
{
    mCheckFailCount[check] += add;
}

namespace Hwcval
{
    class ValCallbacks
    {
    public:
        virtual ~ValCallbacks() {}

        virtual void Exit() = 0;

        static void Set(Hwcval::ValCallbacks* valCallbacks);
        static void DoExit();
        static Hwcval::ValCallbacks* mInstance;
    };
}


#endif // __HwcTestConfig_h__
