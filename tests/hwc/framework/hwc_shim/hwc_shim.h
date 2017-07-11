/****************************************************************************

Copyright (c) Intel Corporation (2014).

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

File Name:      hwc_shim.h

Description:    Base class of Hardware Composer shim for the Hardware Composer
                framework.

Environment:

Notes:

****************************************************************************/


#ifndef __HWC_SHIM_H__
#define __HWC_SHIM_H__

#include <dlfcn.h>
#include <cutils/log.h>
#include <assert.h>

extern "C"
{
    #include "hardware/hardware.h"
    #include "hardware/hwcomposer.h"
    #include <hardware/gralloc.h>
}

#include <xf86drm.h>        //< For structs and types.
#include <xf86drmMode.h>    //< For structs and types.

#include "HwcTestDefs.h"
#include "HwcDrmShimCallback.h"
#include "HwcShimInitializer.h"

#include <utils/String8.h>
#include <utils/String16.h>
#include <utils/Vector.h>
#include <utils/SystemClock.h>
#include <binder/IServiceManager.h>

#include "HwcvalHwc1.h"

// Real Hwc service and log
//#include <HwcService.h>
//#include <IDiagnostic.h>

//#include "hwc_shim_binder.h"

class HwcShimService;
class HwcTestState;
class HwcShim;
class CHwcFrameControl;

struct ShimHwcProcs
{
    hwc_procs_t procs; // must be first member of structure
    hwc_procs_t const *orig_procs;
    HwcShim *shim;
};

class HwcShim : hwc_composer_device_1, public HwcShimInitializer
{
    public:
        /// TODO set this form test
        /// Max call time
        uint64_t callTimeThreshold;
        /// Used to store time before call
        uint64_t callTimeStart;

    private:
        /// Initialize Hwc shim
        int HwcShimInit(void);
        int HwcShimInitDrivers(HwcTestState* state);

        /// Constructor
        HwcShim(const hw_module_t* module);
        /// Destructor
        virtual ~HwcShim();

        // For hooks into real HWC
        /// TODO used?
        void *mLibHwcHandle;

        /// Pointer to hwc device struct
        hwc_composer_device_1 * hwc_composer_device;
        /// pointer to hw_dev handle to hwc
        hw_device_t * hw_dev;

        /// HWC1 interface to test kernel
        Hwcval::Hwc1* mHwc1;

        /// Cast pointer to device struct to HwcShim class
        static HwcShim* GetComposerShim(hwc_composer_device_1 *dev)
        {
            // ALOG_ASSERT(dev);
            return static_cast<HwcShim*>(dev);
        }


        /// Get drm display  info Displays
        void GetDisplayInfo(void);

        // Real HWC service
//        android::sp<intel::vpg::hwc::IService> mHwcService;

        // Call time functions to check the call time of real HWC and drm
        // functions
        /// Record time before function call
        void StartCallTime(void);
        /// Check length of call time
        void EndCallTime(const char * function);


    public:
       /// Implementation of Real HWC - HookOpen
       static int HookOpen(const hw_module_t* module,
                                     const char* name, hw_device_t** hw_device);
       /// Implementation of Real HWC - HookClose
       static int HookClose(struct hw_device_t* device);

       /// Implementation of Real HWC - OnEventControl
       int OnEventControl(int disp, int event, int enabled);

       int EnableVSync(int disp, bool enable);

    public:
        /// Complete initialization of shim in DRM mode
        virtual void HwcShimInitDrm(void);

    private:
        /// Get pointer to functions in Real drm
        int GetFunctionPointer(void * LibHandle, const char * Symbol,
                                    void **FunctionHandle, uint32_t debug);

        // Shim versions of HWC functions
        // function pointers for hwc functions
        /// Implementation of Real HWC - HookPreapre
        static int HookPrepare(struct hwc_composer_device_1 *dev,
                                size_t numDisplays,
                                hwc_display_contents_1_t** displays);

        /// Implementation of Real HWC - HookSet
        static int HookSet(struct hwc_composer_device_1 *dev,
                            size_t numDisplays,
                            hwc_display_contents_1_t** displays);

        /// Implementation of Real HWC - HooEventControl
        static int HookEventControl(struct hwc_composer_device_1 *dev, int disp,
                                    int event, int enabled);
        /// Implementation of Real HWC - HookBlank
        static int HookBlank(struct hwc_composer_device_1 *dev, int disp,
                                        int blank);

        /// Implementation of Real HWC - HookQuery
        static int HookQuery(struct hwc_composer_device_1 *dev, int what,
                                int* value);

        /// Implementation of Real HWC - HookRegisterProcs
        static void HookRegisterProcs(struct hwc_composer_device_1 *dev,
                                        hwc_procs_t const* procs);

        /// Implementation of Real HWC - HookDump
        static void HookDump(struct hwc_composer_device_1 *dev, char *buff,
                                int buff_len);

        /// Implementation of Real HWC - HookGetDisplayConfigs
        static int HookGetDisplayConfigs(struct hwc_composer_device_1 *dev,
                                            int disp, uint32_t* configs,
                                            size_t* numConfigs);

        /// Implementation of Real HWC - HookGetActiveConfig
        static int HookGetActiveConfig(struct hwc_composer_device_1 *dev,
                                            int disp);

        /// Implementation of Real HWC - HookGetDisplayAtrtributes
        static int HookGetDisplayAttributes(struct hwc_composer_device_1 *dev,
                                            int disp, uint32_t config,
                                            const uint32_t* attributes,
                                            int32_t* values);

        // function pointers to real HWC composer
        /// Implementation of Real HWC - OnPrepare
        int OnPrepare(size_t numDisplays, hwc_display_contents_1_t** displays);

        /// Implementation of Real HWC - OnSet
        int OnSet(size_t numDisplays, hwc_display_contents_1_t** displays);

        /// Implementation of Real HWC - OnBlank
        int OnBlank(int disp, int blank);

        /// Implementation of Real HWC - OnQuery
        int OnQuery(int what, int* value);

        /// Implementation of Real HWC - OnRegisterProcs
        void OnRegisterProcs(hwc_procs_t const* procs);

        /// Implementation of Real HWC - OnDump
        void OnDump(char *buff, int buff_len);

        /// Implementation of Real HWC - OnGetDisplayConfigs
        int OnGetDisplayConfigs(int disp, uint32_t* configs,
                                    size_t* numConfigs);

        /// Implementation of Real HWC - OnGetActiveConfig
        int OnGetActiveConfig(int disp);

        /// Implementation of Real HWC - OnGetDisplayAttributes
        int OnGetDisplayAttributes(int disp, uint32_t config,
                                    const uint32_t* attributes,
                                    int32_t* values);

    private:
        ShimHwcProcs        mShimProcs;
        HwcDrmShimCallback  mDrmShimCallback;
};

#endif

