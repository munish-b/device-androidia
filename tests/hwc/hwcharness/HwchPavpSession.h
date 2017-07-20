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
