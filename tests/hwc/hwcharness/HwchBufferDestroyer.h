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
* File Name:            HwchBufferDestroyer.h
*
* Description:          Buffer destroyer thread for testing asynchronous buffer destruction.
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#ifndef __HwchBufferDestroyer_h__
#define __HwchBufferDestroyer_h__

#include "EventThread.h"
#include <ui/GraphicBuffer.h>
#include <utils/StrongPointer.h>
#include "HwchDefs.h"


namespace Hwch
{
    class BufferDestroyer : public EventThread<android::sp<android::GraphicBuffer>, 256 >
    {
        public:
            BufferDestroyer();
            virtual ~BufferDestroyer();

        protected:
            virtual bool threadLoop();
    };
}

#endif // __HwchBufferDestroyer_h__
