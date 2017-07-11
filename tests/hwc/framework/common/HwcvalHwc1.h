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

File Name:      Hwc1IFace.h

Description:    Class definition HWC 1 shim interface class

Environment:

Notes:

****************************************************************************/
#ifndef __Hwc1Iface_h__
#define __Hwc1Iface_h__

// NOTE: HwcTestDefs.h sets defines which are used in the HWC and DRM stack.
// -> to be included before any other HWC or DRM header file.
#include "HwcTestDefs.h"

#include "HwcTestKernel.h"
#include "DrmShimBuffer.h"
#include <utils/Vector.h>

#define EXPORT_API __attribute__ ((visibility("default")))

namespace Hwcval
{
    class EXPORT_API Hwc1
    {
    private:
        /// Pointer to test state
        HwcTestState* mState;
        HwcTestKernel* mTestKernel;

        // Layer validity is stored separately as it is required in onPrepare
        android::Vector<Hwcval::ValidityType> mLayerValidity[HWCVAL_MAX_CRTCS];

        // Current layer lists in the main thread
        Hwcval::LayerList* mContent[HWCVAL_MAX_CRTCS];

        // OnSet sequence number for validation
        uint32_t mHwcFrame;

        // Number of displays with content in OnSet
        uint32_t mActiveDisplays;

    public:

        //-----------------------------------------------------------------------------
        // Constructor & Destructor
        Hwc1();

        // Public interface used by the test

    #ifdef HWCVAL_BUILD_SHIM_HWCSERVICE
        // Shim of HWC IService
        EXPORT_API void SetHwcServiceShim(android::sp<HwcServiceShim> serviceShim);
    #endif // HWCVAL_BUILD_SHIM_HWCSERVICE


        EXPORT_API void CheckOnPrepareEntry(size_t numDisplays, hwc_display_contents_1_t** displays);
        EXPORT_API void CheckOnPrepareExit(size_t numDisplays, hwc_display_contents_1_t** displays);


        /// Notify entry to onSet from HWC Shim
        void EXPORT_API CheckSetEnter(size_t numDisplays, hwc_display_contents_1_t** displays);

        /// Notify exit from OnSet from HWC Shim, and perform checks
        EXPORT_API void CheckSetExit(size_t numDisplays,
                                        hwc_display_contents_1_t** displays);


        /// Checks before HWC is requested to blank the display
        EXPORT_API void CheckBlankEnter(int disp, int blank);
        /// Checks after HWC is requested to blank the display
        EXPORT_API void CheckBlankExit(int disp, int blank);

        // Display config checking
        EXPORT_API void GetDisplayConfigsExit(int disp, uint32_t* configs, size_t numConfigs);
        EXPORT_API void GetActiveConfigExit(uint32_t disp, uint32_t config);
        EXPORT_API void GetDisplayAttributesExit(uint32_t disp, uint32_t config, const uint32_t* attributes, int32_t* values);
    };
}

#endif // __Hwc1Iface_h__
