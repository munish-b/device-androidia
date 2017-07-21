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

#ifndef __HwcTestDisplayControl_h__
#define __HwcTestDisplayControl_h__

#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE

#include "BxService.h"
#include "idisplaycontrol.h"
#include "idisplaymodecontrol.h"

class HwcTestKernel;
class HwcTestCrtc;

using namespace hwcomposer;

class HwcTestDisplayControl //: public BnDisplayControl
    {
public:
    HwcTestDisplayControl(uint32_t disp, sp<IDisplayControl> real, HwcTestKernel* testKernel);
    virtual ~HwcTestDisplayControl();

    /// restore default control (overscan, scale, ...)
    virtual status_t restoreAllDefaults();

    virtual sp<IDisplayOverscanControl> getOverscanControl();
    virtual sp<IDisplayScalingControl> getScalingControl();
    virtual sp<IDisplayModeControl> getModeControl();
    virtual sp<IDisplayBlankControl> getBlankControl();

#ifdef EXPERIMENTAL
    ///
    virtual sp<IColorControl> getBrightnessControl() { return 0; }
    virtual sp<IColorControl> getContrastControl()   { return 0; }
    virtual sp<IColorControl> getGammaControl()      { return 0; }
    virtual sp<IColorControl> getHueControl()        { return 0; }
    virtual sp<IColorControl> getSaturationControl() { return 0; }
#else
    virtual sp<IColorControl> getColorControl(int32_t param);
#endif

#ifdef EXPERIMENTAL
    /// switch into power safe mode (soft disconnect?)
    virtual status_t powerOff(int off)               { return 0; }
#endif

#ifdef HWCVAL_USE_IWIDICONTROL
    virtual sp<IWidiControl> getWidiControl() { return 0; }
#endif

private:
    uint32_t mDisplayIx;
    android::sp<::hwcomposer::IDisplayControl> mReal;
    android::sp<hwcomposer::IDisplayModeControl> mDisplayModeControl;
    HwcTestKernel* mTestKernel;
};

class HwcTestDisplayModeControl //: public BnDisplayModeControl
    {
public:
    HwcTestDisplayModeControl(uint32_t disp, sp<IDisplayModeControl> real, HwcTestKernel* testKernel);
    virtual ~HwcTestDisplayModeControl();

    /// restore default mode
    virtual status_t restorePreferredMode();

    /// query all available modes
    virtual Vector<int> getAvailableModes();

    /// get current mode
    virtual status_t getMode(uint32_t *width, uint32_t *height, uint32_t *refresh, uint32_t *flags, uint32_t *ratio);

    /// set mode
    virtual status_t setMode(uint32_t width, uint32_t height, uint32_t refresh, uint32_t flags, uint32_t ratio);

#ifdef EXPERIMENTAL
    virtual status_t getScaleMode(uint32_t *scale) { return 0; }
    virtual status_t setScaleMode(uint32_t scale)  { return 0; }
#endif
private:
    uint32_t mDisplayIx;
    sp<IDisplayModeControl> mReal;
    HwcTestCrtc* mCrtc;
    HwcTestKernel* mTestKernel;
};


#endif // HWCVAL_BUILD_SHIM_HWCSERVICE

#endif // __HwcTestDisplayControl_h__
