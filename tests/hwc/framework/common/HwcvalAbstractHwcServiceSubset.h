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

File Name:      HwcvalAbstractHwcServiceSubset.h

Description:    Abstract class declaring a subset of the functionality
                of intel::ufo::hwc::services::IService.
                Only functions we need to shim are declared here.
                This is done rather than using the "real" IService.h
                to restrict source code dependency.

Environment:

****************************************************************************/

#ifndef __HwcvalAbstractHwcServiceSubset_h__
#define __HwcvalAbstractHwcServiceSubset_h__

#include <binder/IInterface.h>
#include <binder/Parcel.h>

namespace android
{
    class String8;
};

namespace hwcomposer {

    class IDiagnostic;
    class IDisplayControl;
    class IVideoControl;

} // namespace services

namespace Hwcval
{

    /** Maintenance interface to control HWC activity.
     */
    class AbstractHwcServiceSubset : public android::IInterface
    {
    public:
      virtual android::sp<hwcomposer::IDisplayControl>
      getDisplayControl(uint32_t display) = 0;
      virtual android::sp<hwcomposer::IVideoControl> getVideoControl() = 0;
#ifdef HWCVAL_MDSEXTMODECONTROL
      virtual android::sp<hwcomposer::IMDSExtModeControl>
      getMDSExtModeControl() = 0;
#endif
    };

} // namespace Hwcval

#endif // __HwcvalAbstractHwcServiceSubset_h__
