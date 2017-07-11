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

File Name:      HwcvalServiceManager.cpp

Description:    Validation version of service manager

Environment:

****************************************************************************/
#include "HwcvalServiceManager.h"
#include "HwcTestState.h"
#include "utils/String8.h"

using namespace android;

namespace android
{
    extern android::sp<IServiceManager> gDefaultServiceManager;
}

static android::sp<android::IServiceManager> theRealServiceManager;
static Hwcval::ServiceManager* theHwcvalServiceManager;

android::sp<android::IServiceManager> realServiceManager()
{
    if (theRealServiceManager.get() == 0)
    {
        theRealServiceManager = android::defaultServiceManager();

        if (theHwcvalServiceManager == 0)
        {
            theHwcvalServiceManager = new Hwcval::ServiceManager();
        }

        android::gDefaultServiceManager = theHwcvalServiceManager;
    }

    return theRealServiceManager;
}

Hwcval::ServiceManager* hwcvalServiceManager()
{
    if (theHwcvalServiceManager == 0)
    {
        theHwcvalServiceManager = new Hwcval::ServiceManager();
    }

    return theHwcvalServiceManager;
}


namespace Hwcval
{

IBinder* ServiceManager::onAsBinder()
{
    // Our service manager is not a service - it's for process internal use only.
#if ANDROID_VERSION >= 600
    return IInterface::asBinder(realServiceManager()).get();
#else
    return realServiceManager()->asBinder().get();
#endif
}

/**
 * Retrieve an existing service, blocking for a few seconds
 * if it doesn't yet exist.
 */
sp<IBinder>         ServiceManager::getService( const String16& name) const
{
    sp<IBinder> spBinder = realServiceManager()->getService(name);
    return spBinder;
}

/**
 * Retrieve an existing service, non-blocking.
 */
sp<IBinder>         ServiceManager::checkService( const String16& name) const
{
    return realServiceManager()->checkService(name);
}

/**
 * Register a service.
 */
status_t            ServiceManager::addService( const String16& name,
                                        const sp<IBinder>& service,
                                        bool allowIsolated)
{
    ssize_t ix = mDiversions.indexOfKey(name);

    if (ix >= 0)
    {
        const android::String16& divertName = mDiversions.valueAt(ix);
        HWCLOGI("Hwcval::ServiceManager@%p::addService diverting %s to %s (@%p)",
            this, String8(name).string(), String8(divertName).string(), service.get());

        return realServiceManager()->addService(divertName, service, allowIsolated);
    }
    else
    {
        HWCLOGI("Hwcval::ServiceManager@%p::addService starting normal %s (@%p)",
            this, String8(name).string(), service.get());
        return realServiceManager()->addService(name, service, allowIsolated);
    }
}

/**
 * Return list of all existing services.
 */
Vector<String16>    ServiceManager::listServices()
{
    return realServiceManager()->listServices();
}

//////////////////////////////////////////////////////
// Extras for validation
//////////////////////////////////////////////////////

// If an addService call is made to service name, it will be replaced with divertName
void ServiceManager::Divert(const android::String16& name, const android::String16& divertName)
{
    mDiversions.add(name, divertName);
}

// To be used to add a validation version of a service
// and at the same time, register the name which HWC will use for its diverted service.
status_t ServiceManager::OverrideService(   const android::String16& name,
                                        const android::String16& divertName,
                                        const android::sp<android::IBinder>& service,
                                        bool allowIsolated)
{
    if (mDiversions.indexOfKey(name) >= 0)
    {
        HWCLOGW("ServiceManager::OverrideService Service %s already started. Not repeating.", String8(name).string());
        return android::NO_ERROR;
    }
    else
    {
        HWCLOGI("ServiceManager::OverrideService Starting service %s and registering divert of future %s to %s",
            String8(name).string(), String8(name).string(), String8(divertName).string());
        Divert(name, divertName);
        return realServiceManager()->addService(name, service, allowIsolated);
    }
}

} // namespace Hwcval

