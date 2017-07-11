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
#ifndef __HwchAbstractPavpSession_h__
#define __HwchAbstractPavpSession_h__

#include "HwchDefs.h"
#include <utils/RefBase.h>

namespace Hwch
{
    class AbstractPavpSession : public virtual android::RefBase
    {
        public:
            AbstractPavpSession();
            virtual ~AbstractPavpSession();

            void SetQuiet(bool quiet);
            virtual bool StartProtectedContent() = 0;
            virtual bool ProtectedContentStarted() = 0;

            // Start a PAVP session, returning session ID if successfully created or -1 otherwise
            virtual int32_t StartPavpSession() = 0;

            int32_t GetPavpSessionId();
            uint32_t GetInstanceId();

        protected:
            bool mQuiet;
            uint32_t mPavpSessionId;
            uint32_t mPavpInstance;
    };

    inline void AbstractPavpSession::SetQuiet(bool quiet)
    {
        mQuiet = quiet;
    }

    inline int32_t AbstractPavpSession::GetPavpSessionId()
    {
        return mPavpSessionId;
    }

    inline uint32_t AbstractPavpSession::GetInstanceId()
    {
        return mPavpInstance;
    }
};

#endif // __HwchAbstractPavpSession_h__
