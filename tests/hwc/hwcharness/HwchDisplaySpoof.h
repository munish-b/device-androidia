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
* File Name:            HwchDisplaySpoof.h
*
* Description:          HWC Harness Display Spoof class definition
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#ifndef __HwchDisplaySpoof_h__
#define __HwchDisplaySpoof_h__

#include "HwcTestDisplaySpoof.h"
#include "HwcvalStall.h"
#include "HwchRange.h"

namespace Hwch
{
    class DisplaySpoof : public HwcTestDisplaySpoof
    {
    public:
        DisplaySpoof();
        virtual ~DisplaySpoof();

        virtual void ModifyStatus(uint32_t frameNo, int& ret);

        // Configure failure parameters in frames
        void Configure(const char* str);

    private:
        Range mRange;
    };
}

#endif // __HwchDisplaySpoof_h__
