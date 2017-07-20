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

#ifndef __HwcTestMdsControl_h__
#define __HwcTestMdsControl_h__

#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE
#ifdef HWCVAL_MDSEXTMODECONTROL

#include "BxService.h"
#include "IDisplayControl.h"
#include "IDiagnostic.h"
#include "IMDSExtModeControl.h"

class HwcTestKernel;

using namespace intel::ufo::hwc::services;

class HwcTestMdsControl : public BnMDSExtModeControl
{
public:
    HwcTestMdsControl(sp<IMDSExtModeControl> real, HwcTestKernel* testKernel);
    virtual ~HwcTestMdsControl();
    virtual status_t updateVideoState(int64_t videoSessionId, bool isPrepared);
    virtual status_t updateVideoFPS(int64_t videoSessionId, int32_t fps);
    virtual status_t updateInputState(bool state);

private:
    sp<IMDSExtModeControl> mReal;
    HwcTestKernel* mTestKernel;
};

#endif // HWCVAL_MDSEXTMODECONTROL
#endif // HWCVAL_BUILD_SHIM_HWCSERVICE

#endif // __HwcTestMdsControl_h__
