/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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

