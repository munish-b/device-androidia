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

//#define LOG_NDEBUG 0
#include "MultiDisplayShim.h"

#include "HwcTestKernel.h"
#include "HwcTestLog.h"
#include "HwcTestState.h"

using namespace android::intel;
using namespace android;


namespace Hwcval
{

MultiDisplayCallbackShim::MultiDisplayCallbackShim(const android::sp<IMultiDisplayCallback>& real)
{
    HwcTestState::getInstance()->SetRunningShim(HwcTestState::eMdsShim);
    mReal = real;
}

MultiDisplayCallbackShim::~MultiDisplayCallbackShim()
{
}

status_t MultiDisplayCallbackShim::blankSecondaryDisplay(bool blank)
{
    return mReal->blankSecondaryDisplay(blank);
}

status_t MultiDisplayCallbackShim::updateVideoState(int sessionId, MDS_VIDEO_STATE state)
{
    HWCLOGI("MultiDisplayCallbackShim::updateVideoState session %d state %d",sessionId, state);
    HwcTestState::getInstance()->GetTestKernel()->UpdateVideoStateLegacy(sessionId, state);
    return mReal->updateVideoState(sessionId, state);
}

status_t MultiDisplayCallbackShim::setHdmiTiming(const MDSHdmiTiming& timing)
{
    HwcTestCrtc* crtc = HwcTestState::getInstance()->GetTestKernel()->GetHwcTestCrtcByDisplayIx(HWCVAL_HDMI_DISPLAY_INDEX);
    crtc->SetUserModeStart();
    status_t st = mReal->setHdmiTiming(timing);
    crtc->SetUserModeFinish(st, timing.width, timing.height, timing.refresh, timing.flags, timing.ratio);
    return st;
}

status_t MultiDisplayCallbackShim::setHdmiScalingType(MDS_SCALING_TYPE type)
{
    return mReal->setHdmiScalingType(type);
}

status_t MultiDisplayCallbackShim::setHdmiOverscan(int hValue, int vValue)
{
    return mReal->setHdmiOverscan(hValue, vValue);
}

status_t MultiDisplayCallbackShim::updateInputState(bool state)
{
    HWCLOGI("MultiDisplayCallbackShim::updateInputState %d",state);
    HwcTestState::getInstance()->GetTestKernel()->UpdateInputState(state);
    return mReal->updateInputState(state);
}


// singleton
sp<MultiDisplayCallbackRegistrarShim> MultiDisplayCallbackRegistrarShim::sCbInstance = NULL;

MultiDisplayCallbackRegistrarShim::MultiDisplayCallbackRegistrarShim(const sp<IMultiDisplayCallbackRegistrar>& real)
{
    mReal = real;
    sCbInstance = this;
}

status_t MultiDisplayCallbackRegistrarShim::registerCallback(const sp<IMultiDisplayCallback>& realCbk)
{
    mShimCbk = new MultiDisplayCallbackShim(realCbk);

    if (mReal != 0)
    {
        return mReal->registerCallback(mShimCbk);
    }
    else
    {
        return android::NO_ERROR;
    }
}

status_t MultiDisplayCallbackRegistrarShim::unregisterCallback(const sp<IMultiDisplayCallback>& realCbk)
{
    status_t ret = android::NO_ERROR;
    HWCVAL_UNUSED(realCbk);

    if (mReal != 0)
    {
        ret = mReal->unregisterCallback(mShimCbk);
    }

    mShimCbk = 0;
    return ret;
}

sp<IMultiDisplayCallback> MultiDisplayCallbackRegistrarShim::getCallback()
{
    return mShimCbk;
}


// singleton
sp<MultiDisplayInfoProviderShim> MultiDisplayInfoProviderShim::sInfoInstance = NULL;

MultiDisplayInfoProviderShim::MultiDisplayInfoProviderShim(const sp<IMultiDisplayInfoProvider>& real)
  : mShimSessionId(-1)
{
    mReal = real;
    sInfoInstance = this;
    HwcTestState::getInstance()->SetMDSInfoProviderShim(this);
}

MultiDisplayInfoProviderShim::~MultiDisplayInfoProviderShim()
{
}

void MultiDisplayInfoProviderShim::SetShimVideoSourceInfo(int sessionId, MDSVideoSourceInfo* source)
{
    mShimSessionId = sessionId;
    mShimSource = *source;
}

MDS_VIDEO_STATE MultiDisplayInfoProviderShim::getVideoState(int sessionId)
{
    if (mReal != 0)
    {
        return mReal->getVideoState(sessionId);
    }
    else
    {
        return android::intel::MDS_VIDEO_STATE_UNKNOWN;
    }
}

#ifdef HWCVAL_GETVPPSTATE_EXISTS
#if ANDROID_VERSION >= 500
uint32_t MultiDisplayInfoProviderShim::getVppState()
#else
bool MultiDisplayInfoProviderShim::getVppState()
#endif
{
    if (mReal != 0)
    {
        return mReal->getVppState();
    }
    else
    {
        return false;
    }
}
#endif // HWCVAL_GETVPPSTATE_EXISTS

int MultiDisplayInfoProviderShim::getVideoSessionNumber()
{
    // This function returns the NUMBER OF VIDEO SESSIONS not the number of a particular video session.
    HWCLOGV_COND(eLogVideo, "MultiDisplayInfoProviderShim::getVideoSessionNumber mShimSessionId=%d FPS=%d", mShimSessionId, mShimSource.frameRate);
    if (mShimSessionId >= 0)
    {
        // At this point we lack the ability to track multiple sessions here
        // but as this is only relevant to legacy code, who cares.
        return 1;
    }

    if (mReal != 0)
    {
        return mReal->getVideoSessionNumber();
    }
    else
    {
        return 0;
    }
}

MDS_DISPLAY_MODE MultiDisplayInfoProviderShim::getDisplayMode(bool wait)
{
    if (mReal != 0)
    {
        return mReal->getDisplayMode(wait);
    }
    else
    {
        return MDS_MODE_NONE;   // Technically we are saying that HDMI display is not connected but HWC does not use this info
    }
}

status_t MultiDisplayInfoProviderShim::getVideoSourceInfo(int sessionId, MDSVideoSourceInfo* info)
{
    status_t st = android::NO_ERROR;
    HWCLOGV_COND(eLogVideo, "MultiDisplayInfoProviderShim::getVideoSourceInfo session %d mShimSessionId %d", sessionId, mShimSessionId);

    // Do we match the shim session from the harness test?
    if (sessionId == mShimSessionId)
    {
        *info = mShimSource;
    }
    else if (mReal != 0)
    {
        st = mReal->getVideoSourceInfo(sessionId, info);
    }

    if (st == 0)
    {
        HwcTestState::getInstance()->GetTestKernel()->UpdateVideoFPS(sessionId, info->frameRate);
    }

    return st;
}

status_t MultiDisplayInfoProviderShim::getDecoderOutputResolution(int sessionId, int32_t* width, int32_t* height
#if ANDROID_VERSION >= 500
            , int32_t* offX, int32_t* offY, int32_t* bufW, int32_t* bufH
#endif
            )
{
    if (mReal != 0)
    {
        return mReal->getDecoderOutputResolution(sessionId, width, height
#if ANDROID_VERSION >= 500
                , offX, offY, bufW, bufH
#endif
                );
    }
    else
    {
        return android::NO_ERROR;
    }
}


// singleton
sp<MultiDisplayConnectionObserverShim> MultiDisplayConnectionObserverShim::sConnInstance = NULL;

MultiDisplayConnectionObserverShim::MultiDisplayConnectionObserverShim(const sp<IMultiDisplayConnectionObserver>& real)
{
    mReal = real;
    sConnInstance = this;
}

status_t MultiDisplayConnectionObserverShim::updateWidiConnectionStatus(bool connected)
{
    if (mReal != 0)
    {
        return mReal->updateWidiConnectionStatus(connected);
    }
    else
    {
        return android::NO_ERROR;
    }
}

status_t MultiDisplayConnectionObserverShim::updateHdmiConnectionStatus(bool connected)
{
    if (mReal != 0)
    {
        return mReal->updateHdmiConnectionStatus(connected);
    }
    else
    {
        return android::NO_ERROR;
    }
}



//IMPLEMENT_META_INTERFACE(MDService,"com.intel.MDService.Shim");

MultiDisplayShimService::MultiDisplayShimService(android::sp<IMDService>& mds)
  : mMds(mds)
{
    HWCLOGI("%s: create a MultiDisplay Shim service %p", __func__, this);

    if (mMds != 0)
    {
        mMDSCbRegistrar = mMds->getCallbackRegistrar();
        if (mMDSCbRegistrar.get() == NULL)
        {
            HWCERROR(eCheckMdsBind, "%s: Failed to create real mds callback registrar", __func__);
        }
    }
    new MultiDisplayCallbackRegistrarShim(mMDSCbRegistrar);

    if (mMds != 0)
    {
       mMDSInfoProvider = mMds->getInfoProvider();
        if (mMDSInfoProvider.get() == NULL)
        {
            HWCERROR(eCheckMdsBind, "%s: Failed to create real mds info provider Client", __func__);
        }
    }
    new MultiDisplayInfoProviderShim(mMDSInfoProvider);

    if (mMds != 0)
    {
        mMDSConnObserver = mMds->getConnectionObserver();
        if (mMDSConnObserver.get() == NULL)
        {
            HWCERROR(eCheckMdsBind, "%s: Failed to create real mds connection observer Client", __func__);
        }
    }
    new MultiDisplayConnectionObserverShim(mMDSConnObserver);
}

MultiDisplayShimService::~MultiDisplayShimService() {
    HWCLOGV("%s: MultiDisplayShim service %p is destroyed", __func__, this);
}

void MultiDisplayShimService::instantiate()
{
    sp<IServiceManager> sm = realServiceManager();
    if (sm == NULL) {
        HWCERROR(eCheckMdsBind, "%s: Fail to get service manager - " INTEL_MDSSHIM_SERVICE_NAME " not started",  __func__);
    }

    if (mIsolatedMode)
    {
        android::sp<IMDService> mds;
        if (sm->addService(String16(INTEL_MDSSHIM_SERVICE_NAME),new MultiDisplayShimService(mds)))
        {
            HWCERROR(eCheckMdsBind, "Failed to start %s service", INTEL_MDSSHIM_SERVICE_NAME);
        }
        else
        {
            HWCLOGI("Started %s service", INTEL_MDSSHIM_SERVICE_NAME);
        }
    }
    else
    {
        android::sp<IMDService> mds = interface_cast<IMDService>(
                sm->getService(String16(INTEL_MDS_SERVICE_NAME)));

        if (mds == NULL)
        {
            HWCERROR(eCheckMdsBind, "%s: Fail to get real MDS service - " INTEL_MDSSHIM_SERVICE_NAME " not started", __func__);
        }
        else
        {
            if (sm->addService(String16(INTEL_MDSSHIM_SERVICE_NAME),new MultiDisplayShimService(mds)))
            {
                HWCERROR(eCheckMdsBind, "Failed to start %s service", INTEL_MDSSHIM_SERVICE_NAME);
            }
            else
            {
                HWCLOGI("Started %s service", INTEL_MDSSHIM_SERVICE_NAME);
            }
        }
    }
}

const android::String16 MultiDisplayShimService::descriptor("com.intel.MDService.Shim");
const android::String16& MultiDisplayShimService::getInterfaceDescriptor() const
{
    return descriptor;
}

android::sp<IMDService> MultiDisplayShimService::asInterface(const android::sp<android::IBinder>& obj)
{
    android::sp<IMDService> intr;

    if (obj != 0)
    {
        intr = static_cast<IMDService*>(obj->queryLocalInterface(descriptor).get());
    }
    return intr;
}

sp<IMultiDisplayHdmiControl> MultiDisplayShimService::getHdmiControl()
{
    return mMds->getHdmiControl();
}

sp<IMultiDisplayVideoControl> MultiDisplayShimService::getVideoControl()
{
    return mMds->getVideoControl();
}

sp<IMultiDisplayEventMonitor> MultiDisplayShimService::getEventMonitor()
{
    return mMds->getEventMonitor();
}

sp<IMultiDisplayCallbackRegistrar> MultiDisplayShimService::getCallbackRegistrar()
{
    HWCLOGV("%s", __func__);
    return MultiDisplayCallbackRegistrarShim::getInstance();
}

sp<IMultiDisplaySinkRegistrar> MultiDisplayShimService::getSinkRegistrar()
{
    HWCLOGV("%s", __func__);
    return mMds->getSinkRegistrar();
}

sp<IMultiDisplayInfoProvider> MultiDisplayShimService::getInfoProvider()
{
    return MultiDisplayInfoProviderShim::getInstance();
}

sp<IMultiDisplayConnectionObserver> MultiDisplayShimService::getConnectionObserver() {
    return MultiDisplayConnectionObserverShim::getInstance();
}

sp<IMultiDisplayDecoderConfig> MultiDisplayShimService::getDecoderConfig() {
    return mMds->getDecoderConfig();
}


#ifdef TARGET_HAS_ISV
sp<IMultiDisplayVppConfig> MultiDisplayShimService::getVppConfig() {
    return mMds->getVppConfig();
}
#endif

void MultiDisplayShimService::setIsolatedMode(bool isolatedMode)
{
    mIsolatedMode = isolatedMode;
}

// static initialization
bool MultiDisplayShimService::mIsolatedMode = false;

}; //namespace Hwcval
