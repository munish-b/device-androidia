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
* File Name:            HwcHarness.h
*
* Description:          HWC Test Harness Top Level
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#include "HwchTest.h"
#include <utils/Vector.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include <utils/Mutex.h>
#include "HwcTestLog.h"
#include "HwcTestState.h"
#include "HwcTestDefs.h"
#include "HwcTestUtil.h"
#include "HwchWatchdogThread.h"
#include "HwchDisplaySpoof.h"
#include "HwcvalStatistics.h"

#include <dirent.h>
#include "graphics.h"

class HwcTestRunner : public Hwch::TestParams
{
    private:
        Hwch::Interface& mInterface;
        android::Vector<String8> mTestNames;
        android::Vector<String8> mAvoidNames;
        android::Vector<Hwch::Test*> mTests;
        Hwch::Test* mCurrentTest;
        HwcTestState* mState;

        android::KeyedVector<android::String8, HwcTestResult> mResults;
        uint32_t mNumPasses;
        uint32_t mNumFails;
        android::String8 mFailedTests;
        int64_t mStartTime;
        int64_t mEndTime;
        Hwch::ParamVec mParams;
        android::String8 mLogName;
        android::String8 mHwclogPath;
        bool mBrief;
        bool mNoShims;

        uint32_t mTestNum;
        android::String8 mTestName;
        android::String8 mArgs; // all-test arguments for logging
        bool mAllTests;
        bool mHWCLReplay;
        bool mDSReplay;
        uint32_t mDSReplayNumFrames;
        uint32_t mReplayMatch;
        const char *mReplayFileName;
        bool mReplayNoTiming;
        bool mReplayNoProtected;
        bool mReplayTest;
        float mWatchdogFps;

        Hwch::DisplaySpoof mDisplayFailSpoof;

        Hwcval::Mutex mExitMutex;
        Hwch::WatchdogThread mWatchdog;
        Hwch::System& mSystem;

        // Statistics
        Hwcval::Statistics::Value<double> mRunTimeStat;

    public:
        HwcTestRunner(Hwch::Interface& interface);
        int getargs(int argc, char **argv);
        void SetBufferConfig();
        void SetRunnerParams();
        void EntryPriorityOverride();
        void LogTestResult();
        void LogTestResult(const char* testName, const char* args);
        void WriteCsvFile();
        void WriteDummyCsvFile();
        void ParseCSV(const char* p, android::Vector<android::String8>& sv);
        void CombineFiles(int err);
        int CreateTests();
        int RunTests();
        void CRCTerminate(HwcTestConfig& config);
        void ExitChecks();
        void LogSummary();
        void Lock();
        void Unlock();
        void ConfigureState();

        // Enable/disable display spoof
        void EnableDisplayFailSpoof(const char* str);

        // Configure stalls based on command-line options
        void ConfigureStalls();

        // Configure frames where inputs are to be dumped
        void ConfigureFrameDump();

        // Check version consistency and report
        void ReportVersion();

    private:
        Hwch::Test* NextTest();
        FILE* OpenCsvFile();
        void ConfigureStall(Hwcval::StallType ix, const char* optionName);
        FILE* mStatsFile;
};

