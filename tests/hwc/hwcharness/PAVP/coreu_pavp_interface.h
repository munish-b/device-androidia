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

