/****************************************************************************
*
* Copyright (c) Intel Corporation (2013-2014).
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
* File Name:            test_base.h
*
* Description:          This provide a base class for all hwc tests.
*
* Environment:          See Execution Doxygen page below.
*
*
* Notes:
*
*****************************************************************************/



#ifndef __HWC_TEST_BASE_H__
#define __HWC_TEST_BASE_H__

// TODO get display info DRM base class?

#include "SurfaceSender.h"
#include "display_info.h"

#include "HwcTestDefs.h"
#include "test_binder.h"
#include "HwcTestConfig.h"

#include <utils/SystemClock.h>
#include <cutils/properties.h>
#include <unistd.h>

// to avoid included test_binder.h which will create a linking issue due to two
// IHwcServiceShims existing at link time in test_base.o and hwc_test_test.o
//class BpHwcShimService;
//class android::IHwcShimService;

#define NANO_TO_MICRO 1000

HwcTestResult* HwcGetTestResult();
HwcTestConfig* HwcGetTestConfig();


class HwcTestBase
{
    // TODO Test place holder
    /// \futurework The idea 1

public:
    // Constructor
    HwcTestBase(int argc, char ** argv);

    // Destructor
    virtual ~HwcTestBase();

    /// Main entry point for test implmented in each test.
    /// The implementation should set the test end type and check and create some
    /// surfaces
    virtual int Run(void) = 0;

    /// The test must implement a function set the required shim checks
    /// These are then send by ShimSetUpSendSetChecks implemented in this class.
    virtual int SetChecks(void) = 0;

    /// Set all checks as enabled.
    // This function is provided for convince for use in SetChecks().
    void SetDefaultChecks(void);


    // Test setup functions
    /// Ways the test may end
    enum eTestEndType
    {
        etetNone, // default so this can be test for not being set
        etetFrameCount, // Not currently support frame count needs incrementing
        etetRunTime,
        etetUserDriven,
        etetNumberOfTestTypes
    };

    /// Error  from the test
    enum eTestErrorStatusType
    {
        etestNoError = 0,
        etestBinderError,
        etestUnknownRunType,
        etestIncorrectRunTypeSettingType
    };

    /// predefined test types
    enum eTestLength
    {
        etlTenSeconds   = 10000,
    };

    // Test functions
    /// Set the check required
    int SetCheckEnable(bool enable);
    // Get function
    // eTestEndType GetTestEndType(void);

    /// TODO
    void CheckTestEndType();

    /// Set the  run length of the test
    void SetTestRunTime(int64_t runTimeMs);
    /// Get the current time
    int64_t GetTime(void);

    /// Get the end  type of the test
    eTestEndType GetTestEndType(void);
    /// Set the way to end the test
    void SetTestEndType(eTestEndType type);

    /// Start test inlcudes checks that the test has a sesnibale set up.
    int StartTest(void);
    /// Continue test
    bool ContinueTest(void);
    /// Setup checks in shims
    status_t InitialiseChecks();
    /// Get completion status of checks and turn them off
    void DebriefChecks(bool disableAllChecks = true);
    /// Read configuration from shims
    void GetOldConfig(HwcTestConfig& config);
    /// Turn down logging level to reduce change of unattended system lockup
    void SetLoggingLevelToDefault();

    /// Preserve command-line arguments
    void SetArgs(int argc, char ** argv);

    /// Print test test
    void LogTestResult(HwcTestConfig& config);
    void LogTestResult();

    /// Functions for managing surfaces  in the test
    /// Add a new surface to the SurfaceSenders vector
    int CreateSurface(SurfaceSender::SurfaceSenderProperties sSSP);

    /// Binder functions
    void ConnectToShimBinder(void);

    /// Debug - print surfaces
    void DumpSurfaces(
            SurfaceSender::SurfaceSenderProperties surfaceProperties);

    void PrintArgs() const;

    static HwcTestBase* GetTestBase();     // For logging

    HwcTestConfig& GetConfig();
    HwcTestResult& GetResult();

private:
        /// A list of surface providing objects
        Vector<android::sp<SurfaceSender>* > SurfaceSenders;
        /// A display class to return display properties
        Display* mpDisplay;

protected:

    /// Test name
    android::String8 mTestName;
    /// Binder interface
    android::sp<android::IHwcShimService> mHwcService;

    /// The condition for ending the test
    eTestEndType mTestEndCondition;
    /// Run length of test, need a better name
    uint32_t mTestFrameCount;
    /// Length the of time the test should run
    int64_t mTestRunTime;
    /// true if the test run time has been passed as a command line parameter
    bool mTestRunTimeOverridden;
    /// Updated when sending a frame, rename to Current... ? or something
    uint32_t mFrameCount;
    /// Time a call was made
    int64_t mStartTime;
    /// Current time
    int64_t mCurrentTime;

    /// Command-line argument count
    int mArgc;
    /// Command-line arguments
    char** mArgv;

    /// Disable interface with shims
    bool mNoShims;

    // Check enable components
    bool mValHwc;
    bool mValSf;
    bool mValDisplays;
    bool mValBuffers;

    // Additional check enables
    bool mValHwcComposition;
    bool mValIvpComposition;

    /// Stored pointer to this, for logging
    static HwcTestBase* mTheTestBase;
    /// Test Configuration
    HwcTestConfig mConfig;
    /// Test Result
    HwcTestResult mResult;

};

// For logging
inline HwcTestBase* HwcTestBase::GetTestBase()
{
    return mTheTestBase;
}

inline HwcTestConfig& HwcTestBase::GetConfig()
{
    return mConfig;
}

inline HwcTestResult& HwcTestBase::GetResult()
{
    return mResult;
}


#endif // __HWC_TEST_BASE_H__
