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

File Name:      DrmShimPropertySpoof.cpp

Description:    Functions to manage additional spoofed DRM properties

Environment:

Notes:

****************************************************************************/

#include "DrmShimPropertyManager.h"
#include "HwcTestState.h"
#include "DrmShimChecks.h"
#include <string.h>

#undef LOG_TAG
#define LOG_TAG "DRM_SHIM"

// External references to pointers to real DRM functions
extern void * (*fpDrmMalloc)(int size);

extern drmModeObjectPropertiesPtr (*fpDrmModeObjectGetProperties)(int fd,
                                                        uint32_t object_id,
                                                        uint32_t object_type);

extern void (*fpDrmModeFreeObjectProperties) (drmModeObjectPropertiesPtr ptr);

extern drmModePropertyPtr (*fpDrmModeGetProperty) (int fd, uint32_t propertyId);

DrmShimPropertyManager::PropInfo::PropInfo(const char* n, HwcTestKernel::ObjectClass c)
  : mName(n), mClass(c)
{
}

#define DECLARE_PLANE_PROPERTY_2(ID, NAME) \
    PropInfo(NAME, HwcTestKernel::ePlane),
#define DECLARE_PLANE_PROPERTY(X) \
    PropInfo(#X, HwcTestKernel::ePlane),
#define DECLARE_CRTC_PROPERTY(X) \
    PropInfo(#X, HwcTestKernel::eCrtc),
DrmShimPropertyManager::PropInfo DrmShimPropertyManager::mInfo[] =
{
    #include "DrmShimPropertyList.h"
    PropInfo()
};
#undef DECLARE_PLANE_PROPERTY_2
#undef DECLARE_PLANE_PROPERTY
#undef DECLARE_CRTC_PROPERTY

const uint32_t DrmShimPropertyManager::mNumSpoofProperties = Hwcval::PropertyManager::eDrmPropLast - HWCVAL_SPOOF_PROPERTY_OFFSET;

DrmShimPropertyManager::DrmShimPropertyManager()
  : mFd(0)
{
}

DrmShimPropertyManager::~DrmShimPropertyManager()
{
}

drmModeObjectPropertiesPtr DrmShimPropertyManager::ObjectGetProperties(int fd, uint32_t objectId, uint32_t objectType)
{
    HWCLOGV_COND(eLogNuclear, "DrmShimPropertyManager::ObjectGetProperties fd %d objectId %d objectType 0x%x",
        fd, objectId, objectType);
    drmModeObjectPropertiesPtr props = fpDrmModeObjectGetProperties(fd, objectId, objectType);

#ifndef KERNEL_DRM_NO_GET_PROPERTY_SUPPORT
    return props;
#else /*When Kernel dont have support to pass the supported properties to Userspace*/
    HwcTestKernel::ObjectClass cl = HwcTestKernel::eOther;
    if (mChecks)
    {
        cl = mChecks->GetObjectClass(objectId);
    }
    else
    {
        HWCLOGE_COND(eLogNuclear, "DrmShimPropertyManager::ObjectGetProperties no test kernel");
    }

    if (cl == HwcTestKernel::eOther)
    {
        // Not a CRTC or plane, let's not spoof properties.
        HWCLOGV_COND(eLogNuclear, "objectId %d is not CRTC or plane, no spoofed properties", objectId);

        if (objectType == DRM_MODE_OBJECT_CONNECTOR)
        {
            ProcessConnectorProperties(objectId, props);
        }

        return props;
    }

    uint32_t maxProps = props->count_props + mNumSpoofProperties;
    uint32_t propCount = props->count_props;

    uint32_t* propIds = (uint32_t*) fpDrmMalloc(sizeof(uint32_t) * maxProps);
    uint64_t* propValues = (uint64_t*) fpDrmMalloc(sizeof(uint64_t) * maxProps);
    // Note: drmMalloc clears the allocated memory.

    HWCLOGV_COND(eLogNuclear, "DrmShimPropertyManager::ObjectGetProperties copying %d real properties", props->count_props);
    // Copy the real properties into our extended property list
    // and check there is no clash with the spoof property ids
    memcpy(propIds, props->props, sizeof(uint32_t) * props->count_props);
    for (uint32_t i=0; i<props->count_props; ++i)
    {
        uint32_t prop = props->props[i];
        ALOG_ASSERT((prop < HWCVAL_SPOOF_PROPERTY_OFFSET) || (prop > eDrmPropLast));
        propIds[i] = prop;
    }

    memcpy(propValues, props->prop_values, sizeof(uint64_t) * props->count_props);

    HWCLOGV_COND(eLogNuclear, "DrmShimPropertyManager::ObjectGetProperties generating %d spoof properties", mNumSpoofProperties);

    uint32_t* pProp = propIds + props->count_props;
    for (uint32_t i=0; i<mNumSpoofProperties; ++i)
    {
        if (cl == mInfo[i].mClass)
        {
            uint32_t propId = HWCVAL_SPOOF_PROPERTY_OFFSET + i;
            *pProp++ = propId;

            if (propId == eDrmPlaneProp_type)
            {
                // TODO: Proper way of distinguishing primary and overlay planes
                propValues[propCount] = (objectId == 5) ? DRM_PLANE_TYPE_PRIMARY : DRM_PLANE_TYPE_OVERLAY;
            }

            ++propCount;
        }
    }

    drmModeObjectPropertiesPtr newProps = (drmModeObjectPropertiesPtr) fpDrmMalloc(sizeof(drmModeObjectProperties));
    newProps->count_props = propCount;
    newProps->props = propIds;
    newProps->prop_values = propValues;

    drmModeFreeObjectProperties(props);

    HWCLOGV_COND(eLogNuclear, "DrmShimPropertyManager::ObjectGetProperties returning @%p", newProps);
    return newProps;
#endif
}

void DrmShimPropertyManager::ProcessConnectorProperties(uint32_t connId, drmModeObjectPropertiesPtr props)
{
    ssize_t ix = mDRRSPropIds.indexOfKey(connId);
    if (ix >= 0)
    {
        uint32_t drrsPropId = mDRRSPropIds.valueAt(ix);

        for (uint32_t i=0; i<props->count_props; ++i)
        {
            if (props->props[i] == drrsPropId)
            {
                // This is the DRRS property
                if (HwcTestState::getInstance()->IsOptionEnabled(eOptSpoofDRRS))
                {
                    // We want to spoof, so force the property on
                    props->prop_values[i] = HWCVAL_SEAMLESS_DRRS_SUPPORT;
                }
            }
        }
    }
}

drmModePropertyPtr DrmShimPropertyManager::GetProperty(int fd, uint32_t propertyId)
{
    HWCLOGV_COND(eLogNuclear, "DrmShimPropertyManager::GetProperty fd %d propertyId 0x%x",
        fd, propertyId);

#ifdef DRM_IOCTL_MODE_ATOMIC
    if ((propertyId < HWCVAL_SPOOF_PROPERTY_OFFSET) || (propertyId > eDrmPropLast))
#endif
    {
        // Property id out of spoof range - use normal GetProperty
        drmModePropertyPtr prop = fpDrmModeGetProperty(fd, propertyId);

        if (prop)
        {
            HWCLOGV_COND(eLogNuclear, "DrmShimPropertyManager::GetProperty prop %d %s is not spoofed",
                propertyId, prop->name);
        }
        else
        {
            HWCLOGV_COND(eLogNuclear, "DrmShimPropertyManager::GetProperty prop %d not spoofed, returns NULL",
                propertyId);
        }

        return prop;
    }
#ifdef DRM_IOCTL_MODE_ATOMIC
    else
    {
        uint32_t ix = propertyId - HWCVAL_SPOOF_PROPERTY_OFFSET;

        HWCLOGV_COND(eLogNuclear, "DrmShimPropertyManager::GetProperty prop 0x%x spoofed prop ix %d", propertyId, ix);
        drmModePropertyPtr prop = (drmModePropertyPtr) fpDrmMalloc(sizeof(drmModePropertyRes));
        prop->prop_id = propertyId;
        const char* name = mInfo[ix].mName;
        strcpy(prop->name, name);

        HWCLOGV_COND(eLogNuclear, "DrmShimPropertyManager::GetProperty name %s returning prop @%p", name, prop);
        return prop;
    }
#endif
}


Hwcval::PropertyManager::PropType DrmShimPropertyManager::PropIdToType(uint32_t propId, HwcTestKernel::ObjectClass& propClass)
{
    if ((propId >= HWCVAL_SPOOF_PROPERTY_OFFSET) && (propId < eDrmPropLast))
    {
        // It's already one of our spoof properties, so just return the value
        HWCLOGV_COND(eLogNuclear, "DrmShimPropertyManager::PropIdToType passthrough 0x%x", propId);
        propClass = mInfo[propId - HWCVAL_SPOOF_PROPERTY_OFFSET].mClass;
        return (PropType) propId;
    }
    else
    {
        // This is a "real" DRM property
        // So get the property name and look it up in our list to obtain the enum.
        ALOG_ASSERT(mFd);
        drmModePropertyPtr prop = fpDrmModeGetProperty(mFd, propId);

        for (uint32_t i=0; i<mNumSpoofProperties; ++i)
        {
            if (strcmp(prop->name, mInfo[i].mName) == 0)
            {
                propClass = mInfo[i].mClass;
                HWCLOGV_COND(eLogNuclear, "DrmShimPropertyManager::PropIdToType %d %s -> offset %d", propId, mInfo[i].mName, i);
                return static_cast<Hwcval::PropertyManager::PropType>(HWCVAL_SPOOF_PROPERTY_OFFSET + i);
            }
        }

        return Hwcval::PropertyManager::eDrmPropNone;
    }
}

const char* DrmShimPropertyManager::GetName(PropType pt)
{
    if ((pt >= HWCVAL_SPOOF_PROPERTY_OFFSET) && (pt < eDrmPropLast))
    {
        return mInfo[pt - HWCVAL_SPOOF_PROPERTY_OFFSET].mName;
    }
    else
    {
        // Property id out of spoof range - use normal GetProperty
        drmModePropertyPtr prop = fpDrmModeGetProperty(mFd, pt);

        if (prop)
        {
            static char propName[256];
            strcpy(propName, "Real DRM property: ");
            strcat(propName, prop->name);
            return propName;
        }

        return "Real DRM property";
    }
}

void DrmShimPropertyManager::SetFd(int fd)
{
    mFd = fd;
}

void DrmShimPropertyManager::CheckConnectorProperties(uint32_t connId, uint32_t& connectorAttributes)
{
    drmModeObjectPropertiesPtr props = nullptr;

    props = drmModeObjectGetProperties(mFd, connId, DRM_MODE_OBJECT_CONNECTOR);
    ALOG_ASSERT(props);

    // Find the Id of the property
    for (uint32_t i = 0; i < props->count_props; ++i)
    {
        drmModePropertyPtr prop = drmModeGetProperty(mFd, props->props[i]);
        ALOG_ASSERT(prop);

        if (strcmp(prop->name,"ddr_freq") == 0)
        {
            connectorAttributes |= DrmShimChecks::eDDRFreq;
        }
        else if (strcmp(prop->name, "drrs_capability") == 0)
        {
            // Determine property setting for validation
            switch (props->prop_values[i])
            {
                case HWCVAL_SEAMLESS_DRRS_SUPPORT:
                case HWCVAL_SEAMLESS_DRRS_SUPPORT_SW:
                connectorAttributes |= DrmShimChecks::eDRRS;
            }

            // Save the DRRS property ID, so when HWC asks for it we can change the value
            mDRRSPropIds.replaceValueFor(connId, props->props[i]);
        }

        drmModeFreeProperty(prop);
    }
}

int32_t DrmShimPropertyManager::GetPlanePropertyId(uint32_t plane_id, const char *prop_name)
{
    drmModeObjectPropertiesPtr props = nullptr;
    int32_t prop_id = -1;

    props = drmModeObjectGetProperties(mFd, plane_id, DRM_MODE_OBJECT_PLANE);
    ALOG_ASSERT(props);

    // Find the Id of the property
    for (uint32_t i = 0; i < props->count_props; ++i)
    {
        drmModePropertyPtr prop = nullptr;
        prop = drmModeGetProperty(mFd, props->props[i]);
        ALOG_ASSERT(prop);

        if (!strcmp(prop->name,"type"))
        {
            HWCLOGV_COND(eLogNuclear, "DrmShimPropertyManager::GetPlanePropertyId - %s property for plane %d is: %d",
                prop_name, plane_id, prop->prop_id);
            prop_id = prop->prop_id;
            break;
        }

        drmModeFreeProperty(prop);
    }

    return prop_id;
}

int32_t DrmShimPropertyManager::GetPlaneType(uint32_t plane_id)
{
    drmModeObjectPropertiesPtr props = nullptr;

    int32_t prop_id = GetPlanePropertyId(plane_id, "type");
    if (prop_id == -1)
    {
         HWCLOGV_COND(eLogNuclear, "DrmShimPropertyManager::GetPlaneType - could not find id for 'type' property");
         return -1;
    }

    // Get a pointer to the properties and look for the plane type
    props = drmModeObjectGetProperties(mFd, plane_id, DRM_MODE_OBJECT_PLANE);
    if (!props)
    {
         HWCLOGV_COND(eLogNuclear, "DrmShimPropertyManager::GetPlaneType - could not get properties");
         return -1;
    }

    int32_t plane_type = -1;
    for (uint32_t i = 0; i < props->count_props; ++i)
    {
        if (props->props[i] == (uint32_t)prop_id)
        {
            HWCLOGV_COND(eLogNuclear, "DrmShimPropertyManager::GetPlaneType - 'type' property for plane %d has value: %d",
                plane_id, props->prop_values[i]);
            plane_type = props->prop_values[i];
            break;
        }
    }

    drmModeFreeObjectProperties(props);
    return plane_type;
}
