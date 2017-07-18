/****************************************************************************

Copyright (c) Intel Corporation (2015).

DISCLAIMER OF WARRANTY
NEITHER INTEL NOR ITS SUPPLIERS MAKE ANY REPRESENTATION OR WARRANTY OR
CONDITION OF ANY KIND WHETHER EXPRESS OR IMPLIED (EITHER IN FACT OR BY
OPERATION OF LAW) WITH RESPECT TO THE SOURCE CODE.  INTEL AND ITS SUPPLIERS
EXPRESSLY DISCLAIM ALL WARRANTIES OR CONDITIONS OF MERCHANTABILITY OR
FITNESS FOR A PARTICULAR PURPOSE.  INTEL AND ITS SUPPLIERS DO NOT WARRANT
THAT THE SOURCE CODE IS ERROR-FREE OR THAT OPERATION OF THE SOURCE CODE WILL
BE SECURE OR UNINTERRUPTED AND HEREBY DISCLAIM ANY AND ALL LIABILITY ON
ACCOUNT THEREOF.  THERE IS ALSO NO IMPLIED WARRANTY OF NON-INFRINGEMENT.
SOURCE CODE IS LICENSED TO LICENSEE ON AN "AS IS" BASIS AND NEITHER INTEL
NOR ITS SUPPLIERS WILL PROVIDE ANY SUPPORT, ASSISTANCE, INSTALLATION,
TRAINING OR OTHER SERVICES.  INTEL AND ITS SUPPLIERS WILL NOT PROVIDE ANY
UPDATES, ENHANCEMENTS OR EXTENSIONS.

File Name:      HwcTestDisplayControl.h

Description:    Shim of Multi-display service interface into HWC.

Environment:

****************************************************************************/

#ifndef __HwcTestDisplayControl_h__
#define __HwcTestDisplayControl_h__

#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE

#include "BxService.h"
#include "IDisplayControl.h"
#include "IDisplayModeControl.h"

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
