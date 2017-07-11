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
* File Name:            HwchPavpSession.h
*
* Description:          Hwc harness protected content session class definition
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#ifndef __HwchPavpSession_h__
#define __HwchPavpSession_h__

#include "HwchAbstractPavpSession.h"
#include "utils/Thread.h"

#ifdef HWCVAL_BUILD_PAVP
#include "libpavp.h"
#endif

namespace Hwch
{
    class PavpSession : public AbstractPavpSession, public android::Thread
    {
        public:
            PavpSession();
            virtual ~PavpSession();

            virtual bool StartProtectedContent();
            virtual bool ProtectedContentStarted();

            // Start a PAVP session, returning session ID if successfully created or -1 otherwise
            virtual int32_t StartPavpSession();

        private:

#ifdef HWCVAL_BUILD_PAVP
            // Start a CoreU session on the stated PAVP session and return the instance ID if succesfully created
            // or -1 otherwise
            int32_t StartCoreUSession(int32_t pavpSessionId);
            void CloseCoreU();

            int32_t GetPavpLibHandle(pavp_lib_session **pLibInstance);

            // Create a PAVP session, returning status
            int32_t CreatePavpSession(pavp_lib_session *pLibSession);
            int32_t GetPavpSession(pavp_lib_session *pLibSession, uint32_t& id);
            PAVP_SESSION_STATUS GetPavpSessionStatus(int32_t pavpSessionId);

            // Get the CoreU PAVP instance id of the stated session
            uint32_t GetSessionInstance(int32_t session);
#endif

            // Thread functions
            virtual bool threadLoop();
            virtual android::status_t readyToRun();

            uint32_t mOldPavpInstance;
            bool mProtectedContentStarted;
            int mSessionInstance[HWCVAL_MAX_PROT_SESSIONS];

#ifdef HWCVAL_BUILD_PAVP
            void SetPavpSession(int session, PAVP_SESSION_STATUS status, const char* str);

            COREUINTERFACE* mCoreu;
#endif
    };

};

#endif // __HwchPavpSession_h__
