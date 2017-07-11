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
* File Name:            HwchDisplaySpoof.cpp
*
* Description:          HWC Harness Display Spoof class implementation
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#include "HwchDisplaySpoof.h"
#include "HwcTestState.h"
#include "HwcTestUtil.h"

#include <stdlib.h>

Hwch::DisplaySpoof::DisplaySpoof()
{
}

Hwch::DisplaySpoof::~DisplaySpoof()
{
}

void Hwch::DisplaySpoof::ModifyStatus(uint32_t frameNo, int& ret)
{
    if (mRange.Test(frameNo))
    {
        HWCLOGI("Display fail spoof: return value %d replaced with -1", ret);
        ret = -1;
    }
}

void Hwch::DisplaySpoof::Configure(const char* str)
{
    mRange = Range(str);
}
