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
* File Name:            HwchInternalTests.cpp
*
* Description:          Implementation of test classes
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#include "HwchInternalTests.h"
#include "HwchPngImage.h"
#include "HwchLayers.h"
#include "HwcTestLog.h"
#include "HwcTestState.h"
#include "HwcTestUtil.h"
#include "SSIMUtils.h"


REGISTER_TEST(SSIMCompare)
Hwch::SSIMCompareTest::SSIMCompareTest(Hwch::Interface& interface)
    : Hwch::Test(interface)
{
}

int Hwch::SSIMCompareTest::RunScenario()
{
    const char *filename1 = "SSIM_refimage_1.png";
    const char *filename2 = "SSIM_refimage_2.png";
    int blur_type = ebtLinear;

    if (!strcmp(GetStrParam("blur", "linear"), "gaussian"))
    {
        blur_type = ebtGaussian;
    }

    // load png images

    PngImage *pngimage1 = new Hwch::PngImage();
    if (!pngimage1->ReadPngFile(filename1))
    {
        HWCERROR(eCheckTestFail, "Failed reading input png file\n");
        return 1;
    }

    PngImage *pngimage2 = new Hwch::PngImage();
    if (!pngimage2->ReadPngFile(filename2))
    {
        HWCERROR(eCheckTestFail, "Failed reading input png file\n");
        return 1;
    }

    uint32_t image_width = pngimage1->GetWidth();
    uint32_t image_height = pngimage1->GetHeight();

    if ((image_width  != pngimage2->GetWidth()) ||
        (image_height != pngimage2->GetHeight()))
    {
        HWCERROR(eCheckTestFail, "The two images are different in size. Exit.");
        return 1;
    }


    // SSIM preliminary calculations

    double SSIMIndex = 0;
    dssim_info *dinf = new dssim_info();

    DoSSIMCalculations(dinf, (dssim_rgba**)pngimage1->GetRowPointers(),
                             (dssim_rgba**)pngimage2->GetRowPointers(),
                             image_width, image_height, blur_type);


    // calculate SSIM index averaged on channels

    for (int ch = 0; ch < CHANS; ch++)
    {
        SSIMIndex += GetSSIMIndex(&dinf->chan[ch]);
    }
    SSIMIndex /= (double)CHANS;

    printf("%s SSIM index = %.6f\n", __FUNCTION__, SSIMIndex);


    // deallocations

    delete dinf;
    delete pngimage1;
    delete pngimage2;

    return 0;
}
