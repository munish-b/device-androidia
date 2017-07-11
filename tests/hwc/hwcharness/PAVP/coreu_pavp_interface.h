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
* File Name:            hwcharness/PAVP/coreu_pavp_interface.h
*
* Description:          Harness PAVP include file
*                       Only used when we don't have access to UFO incldues
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#ifndef __HwchPavpIncludes_h__
#define __HwchPavpIncludes_h__

extern "C"
{

#include "PavpTypes.h"
#include "windef4linux.h"
#include "gfxEscape.h"
#include "PavpEscape.h"

// From CoreUPavp.h
// PAVP Test sub-op codes
typedef enum
{
    PAVP_TEST_NOOP              = 0,
    PAVP_TEST_GET_POWERCHECK    = 1,
    PAVP_TEST_SET_POWERCHECK    = 2,
    PAVP_TEST_GET_FORCEBINDER   = 3,
    PAVP_TEST_SET_FORCEBINDER   = 4
} PAVP_TEST_OP;


// From coreu_pavp_interface.h
typedef void COREUINTERFACE;
COREUINTERFACE *CoreUInit();

void CoreUDestroy(
        COREUINTERFACE              *Interface);

bool CoreUDoPavpAction(
        COREUINTERFACE              *Interface,
        void                        *data,
        uint32_t                    dataSize);

bool    CoreUPavpSetMDRMMode (
        COREUINTERFACE                  *Interface,
        bool                            bMultiPavpSessionSupport);

bool    CoreUPavpGetMDRMMode (
        COREUINTERFACE                  *Interface,
        bool                            *bMultiPavpSessionSupport);

bool    CoreUPavpQuerySessionStatus(
        COREUINTERFACE                  *Interface,
        PAVP_SESSION_ID                 SessionID,
        PAVP_SESSION_STATUS             *Status,
        KM_PAVP_SESSION_MODE            *SessionMode,
        uint32_t                        *InstanceCount);

bool    CoreUPavpChangeSession(
        COREUINTERFACE                  *Interface,
        KM_PAVP_SESSION_TYPE            SessionType,
        KM_PAVP_SESSION_MODE            SessionMode,
        PAVP_SESSION_ID                 *SessionID,
        PAVP_SESSION_STATUS             *Status,
        PAVP_SESSION_STATUS             *PrevStatus);
}; // extern "C"


#endif // __HwchPavpIncludes_h__

