/****************************************************************************
*
* Copyright (c) Intel Corporation (2014).
*
* DISCLAIMER OF WARRANTY
* NEITHER INTEL NOR ITS SUPPLIERS MAKE ANY REPRESENTATION OR WARRANTY OR
* CONDITION OF ANY KIND WHETHER EXPRESS OR IMPLIED (EITHER IN FACT OR BY
* OPERATION OF LAW) WITH RESPECT TO THE SOURCE CODE.  INTEL AND ITS SUPPLIERS
* EXPRESSLY DISCLAIM ALL WARRANTIES OR CONDITIONS OF MERCHANTABILITY OR
* FITNESS FOR A PARTICULAR PURPOSE.  INTEL AND ITS SUPPLIERS DO NOT WARRANT
* THAT THE SOURCE CODE IS ERROR-FREE OR THAT OPERATION OF THE SOURCE CODE WILL
* BE SECURE OR UNINTERRUPTED AND HEREBY DISCLAIM ANY AND ALL LIABILITY ON
* ACCOUNT THEREOF.  THERE IS ALSO NO IMPLIED WARRANTY OF NON-INFRINGEMENT.
* SOURCE CODE IS LICENSED TO LICENSEE ON AN "AS IS" BASIS AND NEITHER INTEL
* NOR ITS SUPPLIERS WILL PROVIDE ANY SUPPORT, ASSISTANCE, INSTALLATION,
* TRAINING OR OTHER SERVICES.  INTEL AND ITS SUPPLIERS WILL NOT PROVIDE ANY
* UPDATES, ENHANCEMENTS OR EXTENSIONS.
*
* File Name:            HwchFakePavpSession.cpp
*
* Description:          Hwc harness protected content fake session class implementation
*
* Environment:
*
* Notes:
*
*****************************************************************************/
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
