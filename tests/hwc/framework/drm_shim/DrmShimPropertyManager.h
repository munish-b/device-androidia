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

File Name:      HwcvalPropertySpoof.h

Description:    Functions to manage additional spoofed DRM properties

Environment:

Notes:

****************************************************************************/

#ifndef __DrmShimPropertyManager_h__
#define __DrmShimPropertyManager_h__

#include "HwcvalPropertyManager.h"
#include "HwcvalPropertyManager.h"

extern "C"
{
    #include <xf86drm.h>        //< For structs and types.
    #include <xf86drmMode.h>    //< For structs and types.
};

class DrmShimChecks;

class DrmShimPropertyManager : public Hwcval::PropertyManager
{
private:
    struct PropInfo
    {
        PropInfo(const char* n = 0, HwcTestKernel::ObjectClass c = HwcTestKernel::ePlane);
        const char* mName;
        HwcTestKernel::ObjectClass mClass;
    };

    static PropInfo mInfo[];
    static const uint32_t mNumSpoofProperties;
    int mFd;

    // DRRS property ID per connector id
    android::KeyedVector<uint32_t, uint32_t> mDRRSPropIds;

public:
    DrmShimPropertyManager();
    virtual ~DrmShimPropertyManager();

    drmModeObjectPropertiesPtr ObjectGetProperties(int fd, uint32_t objectId, uint32_t objectType);

    drmModePropertyPtr GetProperty(int fd, uint32_t propertyId);

    virtual PropType PropIdToType(uint32_t propId, HwcTestKernel::ObjectClass& propClass);
    virtual const char* GetName(PropType pt);

    void ProcessConnectorProperties(uint32_t connId, drmModeObjectPropertiesPtr props);
    virtual void CheckConnectorProperties(uint32_t connId, uint32_t& connectorAttributes);
    int32_t GetPlaneType(uint32_t plane_id);
    int32_t GetPlanePropertyId(uint32_t, const char*);

    void SetFd(int fd);
};

#endif // __DrmShimPropertyManager_h__


