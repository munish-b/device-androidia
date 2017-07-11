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
 * @file    Hwch::ReplayDSLayers.cpp
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    3rd July 2014
 * @brief   Implementation of the dumpsys layer profiles.
 *
 * @details This file implements the dumpsys layer profiles that are declared
 * in Hwch::ReplayDSLayers.h.
 *
 *****************************************************************************/

#include "HwchPattern.h"
#include "HwchReplayDSLayers.h"
#include "HwchSystem.h"

Hwch::ReplayDSLayerVideoPlayer::ReplayDSLayerVideoPlayer()
  : Hwch::ReplayLayer("VideoPlayer", 0, 0, HAL_PIXEL_FORMAT_RGBA_8888, 3)
{
    SetPattern(Hwch::GetPatternMgr().CreateHorizontalLinePtn(
                mFormat, 60.0, Hwch::eBlack, Hwch::eLightGreen));
}

Hwch::ReplayDSLayerApplication::ReplayDSLayerApplication()
  : Hwch::ReplayLayer("Application", 0, 0, HAL_PIXEL_FORMAT_RGBA_8888, 3)
{
    SetPattern(Hwch::GetPatternMgr().CreateHorizontalLinePtn(
                mFormat, 60.0, Hwch::eBlack, Hwch::eLightBlue));
}

Hwch::ReplayDSLayerTransparent::ReplayDSLayerTransparent()
  : Hwch::ReplayLayer("Transparent", 0, 0, HAL_PIXEL_FORMAT_RGBA_8888, 3)
{
    SetPattern(Hwch::GetPatternMgr().CreateHorizontalLinePtn(
                mFormat, 60.0, Hwch::eBlack, Alpha(Hwch::eWhite, 1)));
}

Hwch::ReplayDSLayerSemiTransparent::ReplayDSLayerSemiTransparent()
  : Hwch::ReplayLayer("SemiTransparent", 0, 0, HAL_PIXEL_FORMAT_RGBA_8888, 3)
{
    SetPattern(Hwch::GetPatternMgr().CreateHorizontalLinePtn(
                mFormat, 60.0, Hwch::eBlack, Alpha(Hwch::eWhite, 128)));
}

Hwch::ReplayDSLayerStatusBar::ReplayDSLayerStatusBar()
  : Hwch::ReplayLayer("StatusBar", 0, 0, HAL_PIXEL_FORMAT_RGBA_8888, 3)
{
    SetPattern(Hwch::GetPatternMgr().CreateHorizontalLinePtn(
                mFormat, 60.0, Hwch::eBlack, Hwch::eLightPurple));
}

Hwch::ReplayDSLayerNavigationBar::ReplayDSLayerNavigationBar()
  : Hwch::ReplayLayer("NavigationBar", 0, 0, HAL_PIXEL_FORMAT_RGBA_8888, 3)
{
    SetPattern(Hwch::GetPatternMgr().CreateHorizontalLinePtn(
                mFormat, 60.0, Hwch::eBlack, Hwch::eBlue));
}

Hwch::ReplayDSLayerDialogBox::ReplayDSLayerDialogBox()
  : Hwch::ReplayLayer("DialogBox", 0, 0, HAL_PIXEL_FORMAT_RGBA_8888, 3)
{
    SetPattern(Hwch::GetPatternMgr().CreateHorizontalLinePtn(
                mFormat, 60.0, Hwch::eBlack, Hwch::eLightRed));
}
