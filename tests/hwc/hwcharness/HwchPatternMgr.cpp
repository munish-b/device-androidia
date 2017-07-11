/****************************************************************************
*
* Copyright (c) Intel Corporation (2015).
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
* File Name:            HwchPatternMgr.cpp
*
* Description:          class implementation for pattern manager
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#include "HwchPatternMgr.h"
#include "HwchPattern.h"
#include "HwchGlPattern.h"
#include "HwcTestState.h"
#include "HwcTestUtil.h"

Hwch::PatternMgr::PatternMgr()
{
}

Hwch::PatternMgr::~PatternMgr()
{
}

void Hwch::PatternMgr::Configure(bool forceGl, bool forceNoGl)
{
    mForceGl = forceGl;
    mForceNoGl = forceNoGl;
}

bool Hwch::PatternMgr::IsGlPreferred(uint32_t bufferFormat)
{
    if (mForceGl)
    {
        return true;
    }
    else if (mForceNoGl)
    {
        return false;
    }
    else
    {
        switch (bufferFormat)
        {
            case HAL_PIXEL_FORMAT_RGBA_8888:
            case HAL_PIXEL_FORMAT_BGRA_8888:
            case HAL_PIXEL_FORMAT_RGBX_8888:
            case HAL_PIXEL_FORMAT_RGB_565:
                return true;

            default:
                return false;
        }
    }
}

Hwch::Pattern* Hwch::PatternMgr::CreateSolidColourPtn(uint32_t bufferFormat, uint32_t colour, uint32_t flags)
{
    HWCVAL_UNUSED(flags);

    if (IsGlPreferred(bufferFormat))
    {
        return new ClearGlPtn(0, colour, colour);
    }
    else
    {
        return new SolidColourPtn(colour);
    }
}

Hwch::Pattern* Hwch::PatternMgr::CreateHorizontalLinePtn(uint32_t bufferFormat, float updateFreq,
    uint32_t fgColour, uint32_t bgColour, uint32_t matrixColour, uint32_t flags)
{
    HWCVAL_UNUSED(flags);

    if (IsGlPreferred(bufferFormat))
    {
        if (matrixColour != 0)
        {
            return new MatrixGlPtn(updateFreq, fgColour, matrixColour, bgColour);
        }
        else
        {
            return new HorizontalLineGlPtn(updateFreq, fgColour, bgColour);
        }
    }
    else
    {
        return new HorizontalLinePtn(updateFreq, fgColour, bgColour);
    }
}

Hwch::Pattern* Hwch::PatternMgr::CreatePngPtn(uint32_t bufferFormat, float updateFreq, Hwch::PngImage& image,
    uint32_t lineColour, uint32_t bgColour, uint32_t flags)
{
    if (IsGlPreferred(bufferFormat))
    {
        PngGlPtn* ptn = new PngGlPtn(updateFreq, lineColour, bgColour, (flags & ePtnUseIgnore) != 0);
        ptn->Set(image);
        return ptn;
    }
    else
    {
        PngPtn* ptn = new PngPtn(updateFreq, lineColour);
        ptn->Set(image);
        return ptn;
    }
}

