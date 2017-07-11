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

File Name:      HwcShimInitializer.h

Description:    Base class of Hardware Composer shim for the Hardware Composer
                framework.

Environment:

Notes:

****************************************************************************/


#ifndef __HwcShimInitializer_h__
#define __HwcShimInitializer_h__

class HwcShimInitializer
{
    public:
        /// struct of pointer to drm shim function as hwc is linked against real
        /// drm
        typedef struct
        {
            /// pointer to drm shim drmShimInit
            void (*fpDrmShimInit)(bool isHwc, bool isDrm);
            void (*fpDrmShimEnableVSyncInterception)(bool intercept);
            void (*fpDrmShimRegisterCallback)(void* cbk);

        } drmShimFunctionsType;

        /// struct of pointer to real drm functions
        drmShimFunctionsType drmShimFunctions;

        /// struct of pointer to iVP shim function as hwc is linked against real iVP
        typedef struct
        {
            /// pointer to iVP shim iVPShimInit
            void (*fpiVPShimInit)();
            /// pointer to iVP shim iVPShimCleanup
            void (*fpiVPShimCleanup)(void);
        } iVPShimFunctionsType;

        /// struct of pointer to real iVP functions
        iVPShimFunctionsType iVPShimFunctions;

    public:
        virtual ~HwcShimInitializer()
        {
        }

        // pointer to HWC State
        HwcTestState* state;

        /// Complete initialization of shim in DRM mode
        virtual void HwcShimInitDrm(void) = 0;

};

#endif // __HwcShimInitializer_h__

