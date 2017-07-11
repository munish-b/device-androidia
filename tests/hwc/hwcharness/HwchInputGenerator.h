/****************************************************************************
*
* Copyright (c) Intel Corporation (2015).
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
* File Name:            HwchInputGenerator.h
*
* Description:          Keypress generator class definition
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#ifndef __HwchInputGenerator_h__
#define __HwchInputGenerator_h__

#include "utils/Thread.h"
#include "HwchDefs.h"

namespace Hwch
{
    class InputGenerator : public android::Thread
    {
    public:
        InputGenerator();
        virtual ~InputGenerator();

        // Open and configure the ui device
        void Open();

        // Start or stop keypress generation
        void SetActive(bool active);

        // Wait until previous active/inactive request is complete
        void Stabilize();

        // Start or stop keypress generation.
        // If stopping, wait for input to time out.
        void SetActiveAndWait(bool active);

    private:
        // Thread functions
        virtual bool threadLoop();
        virtual android::status_t readyToRun();

        void Keypress();
        int WriteEvent(int type, int code, int value);

        int mFd;
        bool mRunning;
        bool mActive;
        bool mKeypressFailed;

        // Time at which input will have timed out
        int64_t mInactiveTime;

        static const uint32_t mKeypressIntervalUs;
        static const uint32_t mTimeoutPeriodUs;
    };
}

#endif // __HwchInputGenerator_h__
