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
* File Name:            valhwc_nv12_video_full_test.cpp
*
* Description:          Create surfaces used by Android when playing as full
*                       video.
*
* Environment :         See test_base.h for a description of the test
*                       environment.
*
*****************************************************************************/

#define LOG_TAG "HWC_TEST"

#include <cutils/memory.h>

#include <utils/Log.h>
#include <utils/threads.h>
#include <utils/Vector.h>

#include <unistd.h>


#include "test_base.h"

/** \addtogroup HwcTestNV12FullScreenVideo NV12 Full ScreenVideo
    \ingroup UseCaseTests
    @{
        \brief Create a surface to mimic full screen video in NV12 format.
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
    mTestName = "hwc_nv12_video_full_test";
}

int HwcTestTest::SetChecks(void)
{
    SetDefaultChecks();
    return 0;
}

int HwcTestTest::Run(void)
{
    SurfaceSender::SurfaceSenderProperties
        sSSP1(SurfaceSender::epsVideoFullScreenNV12);
    CreateSurface(sSSP1);

    // Set test mode frame or time
    SetTestRunTime(HwcTestBase::etlTenSeconds);
    SetTestEndType(etetRunTime);

    StartTest();

    return mResult.IsGlobalFail() ? 1 : 0;
}

int main (int argc, char ** argv)
{
    HwcTestTest test(argc, argv);

    if(argc == 2 && strcmp(argv[1], "-h") == 0)
    {
        test.PrintArgs();
        return 1;
    }
    return test.Run();
}

