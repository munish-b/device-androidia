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

File Name:      HwcvalPropertyManager.h

Description:    Abstract base class to manage and spoof DRM properties

Environment:

Notes:

****************************************************************************/

#ifndef __HwcvalPropertyManager_h__
#define __HwcvalPropertyManager_h__

#include "HwcTestKernel.h"
class DrmShimChecks;

// We are assuming that DRM will not create properties with ids in the range
// (spoofProprtyOffset) to (spoofPropertyOffset + numberOfProperties)
#define HWCVAL_SPOOF_PROPERTY_OFFSET 0x12340000

namespace Hwcval
{

    class PropertyManager
    {
    public:
        PropertyManager()
          : mChecks(0)
        {
        }

        virtual ~PropertyManager()
        {
        }

        // Generate enum
        #define DECLARE_PLANE_PROPERTY_2(ID, NAME) \
            eDrmPlaneProp_##ID,
        #define DECLARE_PLANE_PROPERTY(X) \
            eDrmPlaneProp_##X,
        #define DECLARE_CRTC_PROPERTY(X) \
            eDrmCrtcProp_##X,
        enum PropType
        {
            eDrmPropNone = HWCVAL_SPOOF_PROPERTY_OFFSET - 1,
            #include "DrmShimPropertyList.h"
            eDrmPropLast
        };
        #undef DECLARE_CRTC_PROPERTY
        #undef DECLARE_PLANE_PROPERTY
        #undef DECLARE_PLANE_PROPERTY_2


        virtual void CheckConnectorProperties(uint32_t connId, uint32_t& attributes) = 0;
        virtual PropType PropIdToType(uint32_t propId, HwcTestKernel::ObjectClass& propClass) = 0;
        virtual const char* GetName(PropType pt) = 0;
        virtual int32_t GetPlaneType(uint32_t plane_id) { HWCVAL_UNUSED(plane_id); return -1; }
        void SetTestKernel(DrmShimChecks *testKernel);

    protected:
        DrmShimChecks* mChecks;
        bool mDRRS;
    };

    inline void PropertyManager::SetTestKernel(DrmShimChecks* checks)
    {
        HWCLOGV_COND(eLogNuclear, "Hwcval::PropertyManager has DrmShimChecks @%p", checks);
        mChecks = checks;
    }
}


#endif // __HwcvalPropertyManager_h__
