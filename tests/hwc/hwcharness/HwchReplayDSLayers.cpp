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

#include "HwchPattern.h"
#include "HwchReplayDSLayers.h"
#include "HwchSystem.h"

Hwch::ReplayDSLayerVideoPlayer::ReplayDSLayerVideoPlayer()
    : Hwch::ReplayLayer(bufHandler, "VideoPlayer", 0, 0, HAL_PIXEL_FORMAT_RGBA_8888, 3) {
  SetPattern(Hwch::GetPatternMgr().CreateHorizontalLinePtn(
      mFormat, 60.0, Hwch::eBlack, Hwch::eLightGreen));
}

Hwch::ReplayDSLayerApplication::ReplayDSLayerApplication()
    : Hwch::ReplayLayer(bufHandler, "Application", 0, 0, HAL_PIXEL_FORMAT_RGBA_8888, 3) {
  SetPattern(Hwch::GetPatternMgr().CreateHorizontalLinePtn(
      mFormat, 60.0, Hwch::eBlack, Hwch::eLightBlue));
}

Hwch::ReplayDSLayerTransparent::ReplayDSLayerTransparent()
    : Hwch::ReplayLayer(bufHandler, "Transparent", 0, 0, HAL_PIXEL_FORMAT_RGBA_8888, 3) {
  SetPattern(Hwch::GetPatternMgr().CreateHorizontalLinePtn(
      mFormat, 60.0, Hwch::eBlack, Alpha(Hwch::eWhite, 1)));
}

Hwch::ReplayDSLayerSemiTransparent::ReplayDSLayerSemiTransparent()
    : Hwch::ReplayLayer(bufHandler, "SemiTransparent", 0, 0, HAL_PIXEL_FORMAT_RGBA_8888,
                        3) {
  SetPattern(Hwch::GetPatternMgr().CreateHorizontalLinePtn(
      mFormat, 60.0, Hwch::eBlack, Alpha(Hwch::eWhite, 128)));
}

Hwch::ReplayDSLayerStatusBar::ReplayDSLayerStatusBar()
    : Hwch::ReplayLayer(bufHandler, "StatusBar", 0, 0, HAL_PIXEL_FORMAT_RGBA_8888, 3) {
  SetPattern(Hwch::GetPatternMgr().CreateHorizontalLinePtn(
      mFormat, 60.0, Hwch::eBlack, Hwch::eLightPurple));
}

Hwch::ReplayDSLayerNavigationBar::ReplayDSLayerNavigationBar()
    : Hwch::ReplayLayer(bufHandler, "NavigationBar", 0, 0, HAL_PIXEL_FORMAT_RGBA_8888, 3) {
  SetPattern(Hwch::GetPatternMgr().CreateHorizontalLinePtn(
      mFormat, 60.0, Hwch::eBlack, Hwch::eBlue));
}

Hwch::ReplayDSLayerDialogBox::ReplayDSLayerDialogBox()
    : Hwch::ReplayLayer(bufHandler, "DialogBox", 0, 0, HAL_PIXEL_FORMAT_RGBA_8888, 3) {
  SetPattern(Hwch::GetPatternMgr().CreateHorizontalLinePtn(
      mFormat, 60.0, Hwch::eBlack, Hwch::eLightRed));
}
