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
* File Name:            HwchFakePavpSession.h
*
* Description:          Hwc harness protected content fake session class definition
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#ifndef __HwchFakePavpSession_h__
#define __HwchFakePavpSession_h__

#include "HwchAbstractPavpSession.h"

// Encryption control
#ifdef HWCVAL_BUILD_HWCSERVICE_API
    #include "HwcServiceApi.h"
#else
    #include "IVideoControl.h"
#endif

#include "utils/Thread.h"

namespace Hwch
{
    class FakePavpSession : public AbstractPavpSession, public android::Thread
    {
        public:
            FakePavpSession();
            virtual ~FakePavpSession();

            virtual bool StartProtectedContent();
            virtual bool ProtectedContentStarted();

            // Start a PAVP session, returning session ID if successfully created or -1 otherwise
            virtual int32_t StartPavpSession();

        private:
            // Thread functions
            virtual bool threadLoop();

            // Private data
#ifdef HWCVAL_BUILD_HWCSERVICE_API
            HWCSHANDLE mHwcsHandle;
#else
            android::sp<intel::ufo::hwc::services::IVideoControl> mVideoControl;
#endif
            bool mProtectedContentStarted;
    };

};

#endif // __HwchFakePavpSession_h__
