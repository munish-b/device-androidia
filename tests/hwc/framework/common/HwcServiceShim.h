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

#ifndef __HwcServiceShim_h__
#define __HwcServiceShim_h__

#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE

#include "BxService.h"
#include "idiagnostic.h"
//#include "IVideoResolutionListener.h"
#include <binder/IInterface.h>

#include "HwcTestUtil.h"
#include "HwcTestState.h"

using namespace hwcomposer;

class HwcTestProtectionChecker;

class HwcServiceShim : public BxService, public Hwcval::Singleton<HwcServiceShim>
{
public:

    // Only override the functions we really want a shim implementation of.
    // Other functions will be passed straight through at binder level.
    //
    // WARNING: If you want to override more functions, you have to
    // 1. add the transact code in BxService.cpp
    // 2. add the binder id to the enum in BxService.h
    // 3. add the pure virtual function declaration to HwcvalAbstractHwcServiceSubset.h.
    virtual sp<IDisplayControl>     getDisplayControl(uint32_t display);
    virtual sp<IVideoControl>       getVideoControl();

    bool Start();

    HwcServiceShim();
    virtual ~HwcServiceShim();

    static const android::String16 descriptor;
    virtual const android::String16& getInterfaceDescriptor() const;

private:
    friend class Hwcval::Singleton<HwcServiceShim>;
    sp<IDisplayControl> mDisplayControls[HWCVAL_MAX_CRTCS];
    sp<IVideoControl> mVideoControl;
#ifdef HWCVAL_MDSEXTMODECONTROL
    sp<IMDSExtModeControl> mMDSExtModeControl;
#endif
};
#endif // HWCVAL_BUILD_SHIM_HWCSERVICE

#endif // __HwcServiceShim_h__
