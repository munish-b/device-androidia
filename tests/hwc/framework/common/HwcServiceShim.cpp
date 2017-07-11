/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2013-2015
 * Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to the
 * source code ("Material") are owned by Intel Corporation or its suppliers or
 * licensors. Title to the Material remains with Intel Corporation or its suppliers
 * and licensors. The Material contains trade secrets and proprietary and confidential
 * information of Intel or its suppliers and licensors. The Material is protected by
 * worldwide copyright and trade secret laws and treaty provisions. No part of the
 * Material may be used, copied, reproduced, modified, published, uploaded, posted,
 * transmitted, distributed, or disclosed in any way without Intels prior express
 * written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel
 * or otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 *
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
    ALOGD("Starting %s in shim", INTEL_HWC_SERVICE_NAME);
    if (hwcvalServiceManager()->OverrideService(String16(INTEL_HWC_SERVICE_NAME), String16(INTEL_HWCREAL_SERVICE_NAME), this, false))
    {
        HWCERROR(eCheckHwcServiceBind, "Failed to start HWC Shim Service (%s)", INTEL_HWC_SERVICE_NAME);
        return false;
    }
    HWCLOGA("Started %s in shim", INTEL_HWC_SERVICE_NAME);
    return true;
}

const android::String16 HwcServiceShim::descriptor("intel.hwc.IService.Shim");
const android::String16& HwcServiceShim::getInterfaceDescriptor() const
{
    return descriptor;
}

sp<IDisplayControl> HwcServiceShim::getDisplayControl(uint32_t display)
{
    if (mDisplayControls[display] == 0)
    {
        HWCLOGD("HwcServiceShim::getDisplayControl(%d) creating display control", display);
        sp<IDisplayControl> realDispControl = Real()->getDisplayControl(display);
        HwcTestKernel* testKernel = HwcTestState::getInstance()->GetTestKernel();

        ALOG_ASSERT(realDispControl.get());
        ALOG_ASSERT(testKernel);
        sp<IDisplayControl> dispControl = new HwcTestDisplayControl(display, realDispControl, testKernel);
        mDisplayControls[display] = dispControl;
    }

    return mDisplayControls[display];
}

sp<IVideoControl> HwcServiceShim::getVideoControl()
{
    sp<IVideoControl> video = Real()->getVideoControl();
    HwcTestKernel* testKernel = HwcTestState::getInstance()->GetTestKernel();

    if (testKernel)
    {
        // We are shimming the IVideoControl
        if (mVideoControl.get() == 0)
        {
            mVideoControl = new HwcTestVideoControl(video, testKernel);
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

status_t HwcTestVideoControl::registerVideoResolutionListener( const sp<IVideoResolutionListener> &vppServiceListener )
{
    return mReal->registerVideoResolutionListener(vppServiceListener);
}

status_t HwcTestVideoControl::unregisterVideoResolutionListener( const sp<IVideoResolutionListener> &vppServiceListener )
{
    return mReal->unregisterVideoResolutionListener(vppServiceListener);
}

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
