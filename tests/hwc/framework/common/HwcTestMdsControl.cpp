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

#include "HwcTestMdsControl.h"
#include "HwcTestState.h"
#include "HwcTestKernel.h"

#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE
#ifdef HWCVAL_MDSEXTMODECONTROL

HwcTestMdsControl::HwcTestMdsControl(sp<IMDSExtModeControl> real, HwcTestKernel* testKernel)
  : mReal(real),
    mTestKernel(testKernel)
{
    HWCLOGV("HwcTestMdsControl created @%p", this);
}

HwcTestMdsControl::~HwcTestMdsControl()
{
}

status_t HwcTestMdsControl::updateVideoState(int64_t videoSessionId, bool isPrepared)
{
    HWCLOGV_COND(eLogVideo, "IMDSExtModeControl::updateVideoState session %" PRIi64 " %s ENTER",
        videoSessionId, isPrepared ? "PREPARED" : "UNPREPARED");
    mTestKernel->UpdateVideoState(videoSessionId, isPrepared);

    status_t st = mReal->updateVideoState(videoSessionId, isPrepared);

    //HWCLOGV_COND(eLogVideo, "IMDSExtModeControl::updateVideoState session %" PRIi64 " %s EXIT",
    //    videoSessionId, isPrepared ? "PREPARED" : "UNPREPARED");

    return st;
}

status_t HwcTestMdsControl::updateVideoFPS(int64_t videoSessionId, int32_t fps)
{
    HWCLOGV_COND(eLogVideo, "IMDSExtModeControl::updateVideoFps session %" PRIi64 " fps %d ENTER",
        videoSessionId, fps);

    mTestKernel->UpdateVideoFPS(videoSessionId, fps);
    status_t st =  mReal->updateVideoFPS(videoSessionId, fps);

    //HWCLOGV_COND(eLogVideo, "IMDSExtModeControl::updateVideoFps session %" PRIi64 " fps %d EXIT",
    //    videoSessionId, fps);

    return st;
}

status_t HwcTestMdsControl::updateInputState(bool state)
{
    HWCLOGV_COND(eLogVideo, "IMDSExtModeControl: updateInputState %d ENTER", state);
    mTestKernel->UpdateInputState(state);
    status_t st = mReal->updateInputState(state);
    //HWCLOGV_COND(eLogVideo, "IMDSExtModeControl: updateInputState %d EXIT", state);
    return st;
}

#endif // HWCVAL_MDSEXTMODECONTROL
#endif // HWCVAL_BUILD_SHIM_HWCSERVICE
