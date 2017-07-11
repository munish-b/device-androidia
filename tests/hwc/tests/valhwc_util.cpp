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
* File Name:            valhwc_monitor_test.cpp
*
* Description:          Enable all test in the hwc and drm shims report the
*                       result after the run time. But do not provide any
*                       surfaces.  This is designed to use the test frame work
*                       when using normally.
*
* Environment :         See test_base.h for a description of the test
*                       environment.
*
*****************************************************************************/

#define LOG_TAG "MONITOR_TEST"

#include <cutils/memory.h>

#include <utils/Log.h>
#include <utils/threads.h>
#include <utils/Vector.h>

#include <unistd.h>

#include "test_base.h"
#include "HwcTestLog.h"

/** \addtogroup  HwcTestMonitor
    \ingroup UseCaseTests
    @{
        \brief A test that sets up the shims for testing with all checks enabled.
                This test provides no surfaces to surface flinger and it is used to
                monitor a normal use of a system and to debug the shims.
    @}
*/

using namespace android;

class HwcTestTest : public HwcTestBase
{

public:
    // Constructor
    HwcTestTest(int argc, char ** argv);

    /// Create surfaces and start test
    int Run(void);

    /// Set checks required by the shims
    int SetChecks(void);
};

HwcTestTest::HwcTestTest(int argc, char ** argv)
: HwcTestBase(argc, argv)
{
    mTestName = "hwc_util";
}

int HwcTestTest::SetChecks(void)
{
    SetDefaultChecks();

    return 0;
}

int HwcTestTest::Run(void)
{
    status_t initOK = InitialiseChecks();

    if (initOK != android::NO_ERROR)
    {
        HWCERROR(eCheckSessionFail, "Binder error: %d", initOK);
        return 1;
    }

    return 0;
}

int main (int argc, char ** argv)
{
    int rc = 0;

    HwcTestTest test(argc, argv);

    test.SetTestEndType(HwcTestBase::etetUserDriven);
    test.SetChecks();

    if (argc > 1)
    {
        if (strcmp(argv[1], "start") == 0)
        {
            printf ("Starting checks and logging\n");
            test.Run();
        }
        else if (strcmp(argv[1], "restart") == 0)
        {
            HwcTestConfig oldConfig;
            test.GetOldConfig(oldConfig);

            printf("Stopping checks\n");
            test.DebriefChecks(false);
            printf ("Restarting checks\n");
            test.Run();

            test.LogTestResult(oldConfig);
        }
        else if (strcmp(argv[1], "stop") == 0)
        {
            HwcTestConfig oldConfig;
            test.GetOldConfig(oldConfig);

            printf("Stopping checks\n");
            test.DebriefChecks();

            // Turn down the logging to standard level now that the testing is complete
            // This may prevent the unattended system from locking up.
            test.SetLoggingLevelToDefault();

            test.LogTestResult(oldConfig);
        }
    }

    return rc;
}
