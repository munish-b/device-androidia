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

File Name:      HwcvalServiceManager.h

Description:    Validation version of service manager

Environment:

****************************************************************************/
#ifndef __HwcvalServiceManager_h__
#define __HwcvalServiceManager_h__

#include <binder/IServiceManager.h>
#include <utils/KeyedVector.h>

namespace Hwcval {

// ----------------------------------------------------------------------

class ServiceManager : public android::IServiceManager
{
public:
    virtual android::IBinder* onAsBinder();
    /**
     * Retrieve an existing service, blocking for a few seconds
     * if it doesn't yet exist.
     */
    virtual android::sp<android::IBinder>         getService( const android::String16& name) const;

    /**
     * Retrieve an existing service, non-blocking.
     */
    virtual android::sp<android::IBinder>         checkService( const android::String16& name) const;

    /**
     * Register a service.
     */
    virtual android::status_t            addService( const android::String16& name,
                                            const android::sp<android::IBinder>& service,
                                            bool allowIsolated = false);

    /**
     * Return list of all existing services.
     */
    virtual android::Vector<android::String16>    listServices();

    //////////////////////////////////////////////////////
    // Extras for validation
    //////////////////////////////////////////////////////

    // If an addService call is made to service name, it will be replaced with divertName
    void Divert(const android::String16& name, const android::String16& divertName);

    // To be used to add a validation version of a service
    // and at the same time, register the name which HWC will use for its diverted service.
    android::status_t OverrideService(   const android::String16& name,
                            const android::String16& divertName,
                            const android::sp<android::IBinder>& service,
                            bool allowIsolated);

private:
    android::KeyedVector<android::String16, android::String16> mDiversions;

};

} // namespace Hwcval

Hwcval::ServiceManager* hwcvalServiceManager();

#endif // __HwcvalServiceManager_h__

