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

#include "HwchFakePavpSession.h"
#include "HwcTestState.h"

#ifndef HWCVAL_BUILD_HWCSERVICE_API
#include "iservice.h"
#include "IVideoControl.h"
using namespace hwcomposer;
#endif

Hwch::FakePavpSession::FakePavpSession()
  :
#ifdef HWCVAL_BUILD_HWCSERVICE_API
    mHwcsHandle(0),
#endif
    mProtectedContentStarted(false)
{
}

Hwch::FakePavpSession::~FakePavpSession()
{
}

bool Hwch::FakePavpSession::StartProtectedContent()
{
    ALOGI("Starting PAVPSession thread");
    run("Hwch::PavpSession", android::PRIORITY_NORMAL);
    return true;
}

bool Hwch::FakePavpSession::ProtectedContentStarted()
{
    // Wait for initialization, if still running.
    join();

    return mProtectedContentStarted;
}

// Start a fake PAVP session, returning the session ID and ensuring HWC knows which session is valid
int32_t Hwch::FakePavpSession::StartPavpSession()
{
#ifdef HWCVAL_BUILD_HWCSERVICE_API
    if (mHwcsHandle == 0)
    {
        mHwcsHandle = HwcService_Connect();
    }

    if (mProtectedContentStarted)
    {
        HwcService_Video_DisableEncryptedSession(mHwcsHandle, mPavpSessionId);
    }
#else
    if (mVideoControl.get() == 0)
    {
        // Find and connect to HWC service
      sp<IBinder> hwcBinder =
          defaultServiceManager()->getService(String16(IA_HWC_SERVICE_NAME));
        sp<IService> hwcService = interface_cast<IService>(hwcBinder);
        if(hwcService == NULL)
        {
          HWCERROR(eCheckSessionFail, "Could not connect to service %s",
                   IA_HWC_SERVICE_NAME);
            ALOG_ASSERT(0);
        }

        // Get MDSExtModeControl interface.
        // mVideoControl = hwcService->getVideoControl();
        if (mVideoControl == NULL)
        {
            HWCERROR(eCheckSessionFail, "Cannot obtain IVideoControl");
            ALOG_ASSERT(0);
        }
    }

    if (mProtectedContentStarted)
    {
        mVideoControl->disableEncryptedSession(mPavpSessionId);
    }
#endif

    mPavpSessionId = (mPavpSessionId + 1) % 15;
    ++mPavpInstance;

#ifdef HWCVAL_BUILD_HWCSERVICE_API
    ALOG_ASSERT(mHwcsHandle);

    HwcService_Video_EnableEncryptedSession(mHwcsHandle, mPavpSessionId, mPavpInstance);
#else
    mVideoControl->enableEncryptedSession(mPavpSessionId, mPavpInstance);
#endif

    HWCLOGA("Fake PAVP session %d instance %d started", mPavpSessionId, mPavpInstance);

    return mPavpSessionId;
}

bool Hwch::FakePavpSession::threadLoop()
{
    StartPavpSession();
    mProtectedContentStarted = true;

    // One-shot
    return false;
}
