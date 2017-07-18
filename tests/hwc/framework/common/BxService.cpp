/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2013-2015 Intel Corporation All Rights Reserved.
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

#include "BxService.h"
#include "IVideoControl.h"
#include "IDisplayControl.h"
#ifdef HWCVAL_MDSEXTMODECONTROL
#include "IMDSExtModeControl.h"
#endif
#include <binder/IServiceManager.h>
#include "HwcTestDefs.h"
#include "HwcTestState.h"

using namespace hwcomposer;

BxService::BxService()
  : mRealBinder(0),
    mRealService(0)
{
}

IBinder* BxService::onAsBinder()
{
    return this;
}

void BxService::GetRealService()
{
    // A service may be requested (particularly diagnostic) before the real HWC service has started.
    // If this happens, wait for it, within reason.
    uint32_t tries=0;
    while ((mRealService.get() == 0) && (++tries < 50))
    {
      mRealService =
          android::interface_cast<IService>(realServiceManager()->getService(
              android::String16(IA_HWCREAL_SERVICE_NAME)));
        if (mRealService.get())
        {
#if ANDROID_VERSION >= 600
            mRealBinder = IInterface::asBinder(mRealService);
#else
            mRealBinder = mRealService->asBinder();
#endif
            return;
        }

        if ((tries % 10) == 1)
        {
          HWCLOGI("BxService: retrying after attempt %d to get service %s.",
                  tries, IA_HWCREAL_SERVICE_NAME);
        }

        usleep(HWCVAL_USLEEP_100MS);
    }
    ALOG_ASSERT(mRealService.get());
}

android::sp<IService> BxService::Real()
{
    return mRealService;
}


android::status_t BxService::onTransact(uint32_t code,
                                        const android::Parcel& data,
                                        android::Parcel* reply,
                                        uint32_t flags)
{
    switch(code)
    {
        case TRANSACT_GET_DIAGNOSTIC:
        {
            if (mRealService.get() == 0)
            {
              mRealService = android::interface_cast<IService>(
                  realServiceManager()->getService(
                      android::String16(IA_HWCREAL_SERVICE_NAME)));
            }

            if (mRealService.get() == 0)
            {
                reply->writeStrongBinder(0);
                return android::NO_ERROR;
            }
            else
            {
#if ANDROID_VERSION >= 600
                mRealBinder = IInterface::asBinder(mRealService);
#else
                mRealBinder = mRealService->asBinder();
#endif
                android::status_t st = mRealBinder->transact(code, data, reply, flags);
                return st;
            }
        }
        case TRANSACT_GET_DISPLAY_CONTROL:
        {
            data.checkInterface(this);
            GetRealService();
            uint32_t display = data.readInt32();
#if ANDROID_VERSION >= 600
            android::sp<IBinder> b =
                NULL; // this->IInterface::asBinder(getDisplayControl(display));
#else
            android::sp<IBinder> b = this->getDisplayControl(display)->asBinder();
#endif
            reply->writeStrongBinder(b);
            return android::NO_ERROR;
        }
        case TRANSACT_GET_VIDEO_CONTROL:
        {
            data.checkInterface(this);
            GetRealService();
#if ANDROID_VERSION >= 600
            android::sp<IBinder> b = this->IInterface::asBinder(getVideoControl());
#else
            android::sp<IBinder> b = this->getVideoControl()->asBinder();
#endif
            reply->writeStrongBinder(b);
            return android::NO_ERROR;
        }
#ifdef HWCVAL_MDSEXTMODECONTROL
        case TRANSACT_GET_MDS_EXT_MODE_CONTROL:
        {
            data.checkInterface(this);
            GetRealService();
#if ANDROID_VERSION >= 600
            android::sp<IBinder> b = this->IInterface::asBinder(getMDSExtModeControl());
#else
            android::sp<IBinder> b = this->getMDSExtModeControl()->asBinder();
#endif
            reply->writeStrongBinder(b);
            return android::NO_ERROR;
        }
#endif // HWCVAL_MDSEXTMODECONTROL
        default:
        {
            if ((code >> 24) == '_')
            {
                // IBinder stuff
                // Pass to BBinder
                android::status_t st = BBinder::onTransact(code, data, reply, flags);
                return st;
            }
            else
            {
                // Forward anything else to real HWC service
                // This method should maximise binary compatibility and avoid any problems
                // when HWC adds methods.

                GetRealService();
                android::status_t st = mRealBinder->transact(code, data, reply);
                return st;
            }
        }
    }
}


