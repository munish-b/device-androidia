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

#ifndef __MultiDisplayShim_h__
#define __MultiDisplayShim_h__

//#define LOG_NDEBUG 0
#include "MultiDisplayShimService.h"

using namespace android::intel;
using namespace android;

namespace Hwcval
{

class MultiDisplayCallbackShim : public android::intel::BnMultiDisplayCallback
{
public:
    MultiDisplayCallbackShim(const android::sp<IMultiDisplayCallback>& real);
    virtual ~MultiDisplayCallbackShim();

    status_t blankSecondaryDisplay(bool blank);
    status_t updateVideoState(int sessionId, MDS_VIDEO_STATE state);
    status_t setHdmiTiming(const MDSHdmiTiming& timing);
    status_t setHdmiScalingType(MDS_SCALING_TYPE type);
    status_t setHdmiOverscan(int hValue, int vValue);
    status_t updateInputState(bool state);
private:
    android::sp<IMultiDisplayCallback> mReal;
};


class MultiDisplayCallbackRegistrarShim : public android::intel::BnMultiDisplayCallbackRegistrar
{
private:
    sp<IMultiDisplayCallbackRegistrar> mReal;
    static sp<MultiDisplayCallbackRegistrarShim> sCbInstance;
    android::sp<IMultiDisplayCallback> mShimCbk;

public:
    MultiDisplayCallbackRegistrarShim(const sp<IMultiDisplayCallbackRegistrar>& real);
    status_t registerCallback(const sp<IMultiDisplayCallback>&);
    status_t unregisterCallback(const sp<IMultiDisplayCallback>&);
    static sp<MultiDisplayCallbackRegistrarShim> getInstance()
    {
        return sCbInstance;
    }

    // Not part of IMultiDisplayCallbackRegistrar
    sp<IMultiDisplayCallback> getCallback();
};


class MultiDisplayInfoProviderShim : public BnMultiDisplayInfoProvider
{
private:
    static sp<MultiDisplayInfoProviderShim> sInfoInstance;
    sp<IMultiDisplayInfoProvider> mReal;

public:
    MultiDisplayInfoProviderShim(const sp<IMultiDisplayInfoProvider>& real);
    virtual ~MultiDisplayInfoProviderShim();

    // Test-only functionality
    void SetShimVideoSourceInfo(int sessionId, MDSVideoSourceInfo* source);

    // Shimmed functionality
    MDS_VIDEO_STATE getVideoState(int);

#ifdef HWCVAL_GETVPPSTATE_EXISTS
#if ANDROID_VERSION >= 500
    uint32_t getVppState();
#else
    bool getVppState();
#endif
#endif // HWCVAL_GETVPPSTATE_EXISTS

    int getVideoSessionNumber();
    MDS_DISPLAY_MODE getDisplayMode(bool);
    status_t getVideoSourceInfo(int, MDSVideoSourceInfo*);
    status_t getDecoderOutputResolution(int, int32_t* width, int32_t* height
#if ANDROID_VERSION >= 500
            , int32_t* offX, int32_t* offY, int32_t* bufW, int32_t* bufH
#endif
            );
    static sp<MultiDisplayInfoProviderShim> getInstance()
    {
        return sInfoInstance;
    }

private:
    int mShimSessionId;
    MDSVideoSourceInfo mShimSource;
};


class MultiDisplayConnectionObserverShim : public BnMultiDisplayConnectionObserver
{
private:
    sp<IMultiDisplayConnectionObserver> mReal;
    static sp<MultiDisplayConnectionObserverShim> sConnInstance;
public:
    MultiDisplayConnectionObserverShim(const sp<IMultiDisplayConnectionObserver>& real);
    status_t updateWidiConnectionStatus(bool);
    status_t updateHdmiConnectionStatus(bool);
    static sp<MultiDisplayConnectionObserverShim> getInstance()
    {
        return sConnInstance;
    }
};

}; //namespace Hwcval

#endif // __MultiDisplayShim_h__
