/*
 * Copyright (c) 2014, Intel Corporation. All rights reserved.
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
 *
 * Author: robert.pinsker@intel.com
 */

#ifndef __ANDROID_INTEL_MULTIDISPLAYSHIMSERVICE_H__
#define __ANDROID_INTEL_MULTIDISPLAYSHIMSERVICE_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wwrite-strings"
#include <MultiDisplayService.h>
#pragma GCC diagnostic pop

#ifdef TARGET_HAS_VPP
#define TARGET_HAS_ISV
#endif

namespace Hwcval
{

#define INTEL_MDSSHIM_SERVICE_NAME "display.intel.mds.shim"

class MultiDisplayShimService : public android::intel::BnMDService {
public:
    MultiDisplayShimService(android::sp<IMDService>& mds);
    ~MultiDisplayShimService();

    static const char* getServiceName() { return INTEL_MDSSHIM_SERVICE_NAME; }
    static void instantiate();

    static const android::String16 descriptor;
    virtual const android::String16& getInterfaceDescriptor() const;

    static android::sp<android::intel::IMDService> asInterface(const android::sp<android::IBinder>& obj);

    static void setIsolatedMode(bool isolatedMode);

    virtual android::sp<android::intel::IMultiDisplayHdmiControl>         getHdmiControl();
    virtual android::sp<android::intel::IMultiDisplayVideoControl>        getVideoControl();
    virtual android::sp<android::intel::IMultiDisplayEventMonitor>        getEventMonitor();
    virtual android::sp<android::intel::IMultiDisplayCallbackRegistrar>   getCallbackRegistrar();
    virtual android::sp<android::intel::IMultiDisplaySinkRegistrar>       getSinkRegistrar();
    virtual android::sp<android::intel::IMultiDisplayInfoProvider>        getInfoProvider();
    virtual android::sp<android::intel::IMultiDisplayConnectionObserver>  getConnectionObserver();
    virtual android::sp<android::intel::IMultiDisplayDecoderConfig>       getDecoderConfig();
#ifdef TARGET_HAS_ISV
    virtual android::sp<android::intel::IMultiDisplayVppConfig>           getVppConfig();
#endif
private:
    android::sp<android::intel::IMDService> mMds;

    android::sp<android::intel::IMultiDisplayCallbackRegistrar> mMDSCbRegistrar;
    android::sp<android::intel::IMultiDisplayInfoProvider> mMDSInfoProvider;
    android::sp<android::intel::IMultiDisplayConnectionObserver> mMDSConnObserver;

    static bool mIsolatedMode;
};

}; // namespace Hwcval

#endif
