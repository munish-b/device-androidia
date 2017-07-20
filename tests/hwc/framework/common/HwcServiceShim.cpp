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
#include "HwcServiceShim.h"

#ifdef HWCVAL_BUILD_SHIM_HWCSERVICE

#include <binder/IInterface.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include "HwcTestKernel.h"
#include "HwcvalServiceManager.h"
#include "HwcTestMdsControl.h"
#include "HwcTestDisplayControl.h"

HwcServiceShim::HwcServiceShim()
{
    HWCLOGV("HwcServiceShim created @%p", this);
    memset(mDisplayControls, 0, sizeof(mDisplayControls));
}

HwcServiceShim::~HwcServiceShim()
{
}

bool HwcServiceShim::Start()
{
  ALOGD("Starting %s in shim", IA_HWC_SERVICE_NAME);
  if (hwcvalServiceManager()->OverrideService(String16(IA_HWC_SERVICE_NAME),
                                              String16(IA_HWCREAL_SERVICE_NAME),
                                              this, false)) {
    HWCERROR(eCheckHwcServiceBind, "Failed to start HWC Shim Service (%s)",
             IA_HWC_SERVICE_NAME);
        return false;
    }
    HWCLOGA("Started %s in shim", IA_HWC_SERVICE_NAME);
    return true;
}

const android::String16 HwcServiceShim::descriptor("IA.IService.Shim");
const android::String16& HwcServiceShim::getInterfaceDescriptor() const
{
    return descriptor;
}

sp<IDisplayControl> HwcServiceShim::getDisplayControl(uint32_t display)
{
    if (mDisplayControls[display] == 0)
    {
        HWCLOGD("HwcServiceShim::getDisplayControl(%d) creating display control", display);
        sp<IDisplayControl> realDispControl =
            NULL; // Real()->GetDisplayControl(display);
        HwcTestKernel* testKernel = HwcTestState::getInstance()->GetTestKernel();

        ALOG_ASSERT(realDispControl.get());
        ALOG_ASSERT(testKernel);
        // sp<IDisplayControl> dispControl = new HwcTestDisplayControl(display,
        // realDispControl, testKernel);
        // imDisplayControls[display] = dispControl;
    }

    return mDisplayControls[display];
}

sp<IVideoControl> HwcServiceShim::getVideoControl()
{
  sp<IVideoControl> video = NULL; // Real()->GetVideoControl();
    HwcTestKernel* testKernel = HwcTestState::getInstance()->GetTestKernel();

    if (testKernel)
    {
        // We are shimming the IVideoControl
        if (mVideoControl.get() == 0)
        {
          // mVideoControl = new HwcTestVideoControl(video, testKernel);
        }

        return mVideoControl;
    }
    else
    {
        // Not shimming (shims not installed)
        return video;
    }
}

#ifdef HWCVAL_MDSEXTMODECONTROL
sp<IMDSExtModeControl> HwcServiceShim::getMDSExtModeControl()
{
    if (mMDSExtModeControl.get())
    {
        return mMDSExtModeControl;
    }

    HWCLOGV_COND(eLogVideo, "HwcServiceShim Creating shim IMDSExtModeControl");
    sp<IMDSExtModeControl> mds = Real()->getMDSExtModeControl();
    HwcTestKernel* testKernel = HwcTestState::getInstance()->GetTestKernel();

    if (testKernel)
    {
        // We are shimming the IMDSExtModeControl
        if (mMDSExtModeControl.get() == 0)
        {
            mMDSExtModeControl = new HwcTestMdsControl(mds, testKernel);
        }

        return mMDSExtModeControl;
    }
    else
    {
        // Not shimming (shims not installed)
        return mds;
    }
}
#endif // HWCVAL_MDSEXTMODECONTROL
#if 0
HwcTestVideoControl::HwcTestVideoControl(sp<IVideoControl> real, HwcTestKernel* testKernel)
  : mReal(real),
    mTestKernel(testKernel),
    mProtChecker(testKernel->GetProtectionChecker())
{
    testKernel->SetVideoControl(this);
}

HwcTestVideoControl::~HwcTestVideoControl()
{
}
#endif
status_t HwcTestVideoControl::enableEncryptedSession( uint32_t sessionID, uint32_t instanceID )
{
    mProtChecker.EnableEncryptedSession(sessionID, instanceID);
    int64_t startTime = systemTime(SYSTEM_TIME_MONOTONIC);
    status_t st = mReal->enableEncryptedSession(sessionID, instanceID);
    int64_t ns = systemTime(SYSTEM_TIME_MONOTONIC) - startTime;

    HWCCHECK(eCheckProtEnableStall);
    if (ns > (15 * HWCVAL_MS_TO_NS))
    {
        HWCERROR(eCheckProtEnableStall, "Enabling encrypted session %d instance %d took %fms",
            sessionID, instanceID, double(ns)/double(HWCVAL_MS_TO_NS));
    }

    return st;
}

status_t HwcTestVideoControl::disableEncryptedSession( uint32_t sessionID )
{
    mProtChecker.DisableEncryptedSessionEntry(sessionID);
    int64_t startTime = systemTime(SYSTEM_TIME_MONOTONIC);

    status_t st = mReal->disableEncryptedSession(sessionID);

    mProtChecker.DisableEncryptedSessionExit(sessionID, startTime, systemTime(SYSTEM_TIME_MONOTONIC));
    mTestKernel->CheckInvalidSessionsDisplayed();

    return st;
}

status_t HwcTestVideoControl::disableAllEncryptedSessions( )
{
    mProtChecker.DisableAllEncryptedSessionsEntry();
    status_t st = mReal->disableAllEncryptedSessions();

    mProtChecker.DisableAllEncryptedSessionsExit();
    mTestKernel->CheckInvalidSessionsDisplayed();

    return st;
}

bool HwcTestVideoControl::isEncryptedSessionEnabled( uint32_t sessionID, uint32_t instanceID )
{
    return mReal->isEncryptedSessionEnabled(sessionID, instanceID);
}
#if 0
status_t HwcTestVideoControl::registerVideoResolutionListener( const sp<IVideoResolutionListener> &vppServiceListener )
{
    return mReal->registerVideoResolutionListener(vppServiceListener);
}

status_t HwcTestVideoControl::unregisterVideoResolutionListener( const sp<IVideoResolutionListener> &vppServiceListener )
{
    return mReal->unregisterVideoResolutionListener(vppServiceListener);
}
#endif
status_t HwcTestVideoControl::updateStatus( EDisplayId display, EDisplayStatus status )
{
    return mReal->updateStatus(display, status);
}

#ifdef HWCVAL_VIDEOCONTROL_OPTIMIZATIONMODE
status_t HwcTestVideoControl::setOptimizationMode( EOptimizationMode mode )
{
    mTestKernel->CheckSetOptimizationModeEnter(mode);
    status_t st = mReal->setOptimizationMode(mode);
    mTestKernel->CheckSetOptimizationModeExit(st, mode);

    return st;
}
#endif

#endif // HWCVAL_BUILD_SHIM_HWCSERVICE
