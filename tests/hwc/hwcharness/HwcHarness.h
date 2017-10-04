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

class HwcTestRunner : public Hwch::TestParams {
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
  android::String8 mArgs;  // all-test arguments for logging
  bool mAllTests;
  bool mHWCLReplay;
  bool mDSReplay;
  uint32_t mDSReplayNumFrames;
  uint32_t mReplayMatch;
  const char* mReplayFileName;
  bool mReplayNoTiming;
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
  int getargs(int argc, char** argv);
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
