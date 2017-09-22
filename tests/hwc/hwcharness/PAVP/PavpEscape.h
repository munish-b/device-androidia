/*===================== begin_copyright_notice
==================================

INTEL CONFIDENTIAL
Copyright 2013
Intel Corporation All Rights Reserved.

The source code contained or described herein and all documents related to the
source code ("Material") are owned by Intel Corporation or its suppliers or
licensors. Title to the Material remains with Intel Corporation or its suppliers
and licensors. The Material contains trade secrets and proprietary and
confidential
information of Intel or its suppliers and licensors. The Material is protected
by
worldwide copyright and trade secret laws and treaty provisions. No part of the
Material may be used, copied, reproduced, modified, published, uploaded, posted,
transmitted, distributed, or disclosed in any way without Intel's prior express
written permission.

No license under any patent, copyright, trade secret or other intellectual
property right is granted to or conferred upon you by disclosure or delivery
of the Materials, either expressly, by implication, inducement, estoppel or
otherwise. Any license under such intellectual property rights must be express
and approved by Intel in writing.

======================= end_copyright_notice
==================================*/

#ifndef __PAVP_ESCAPE_H
#define __PAVP_ESCAPE_H

//#include "gfxEscape.h"
//#include "pavp_types.h"

//===========================================================================
// typedef:
//        PAVP_ESC_QUERY_CAPS_PARAMS
//
// Description:
//     Returns hardware PAVP capabilities and settings to UMD.
//---------------------------------------------------------------------------
typedef enum {
  PAVP_ESC_SYSTEM_PAVP_STATUS_NONE,
  PAVP_ESC_SYSTEM_PAVP_STATUS_LITE,
  PAVP_ESC_SYSTEM_PAVP_STATUS_HEAVY
} PAVP_ESC_SYSTEM_PAVP_STATUS;

typedef struct tagPAVP_ESC_QUERY_CAPS_PARAMS {
  PAVP_ESC_SYSTEM_PAVP_STATUS
      AvailablePavpMode;  // Indicates if system is heavy mode capable
} PAVP_ESC_QUERY_CAPS_PARAMS;

//===========================================================================
// typedef:
//        PAVP_ESC_ACCESS_GET_PARAMS
//
// Description:
//     This structure is for retrieving PAVP session counts.
//---------------------------------------------------------------------------
typedef struct tagPAVP_ESC_GET_PARAMS {
  ULONG SupportedDecodeCount;
  ULONG SupportedTranscodeCount;
} PAVP_ESC_GET_PARAMS;

//===========================================================================
// typedef:
//        PAVP_ESC_GET_SET_OPTIONS
//
// Description:
//     This structure is for set/get of platform specific PAVP options
//---------------------------------------------------------------------------
typedef struct tagPAVP_ESC_GET_SET_OPTIONS {
  ULONG OptionOperation;  // Platform specific option
  ULONG Value[4];         // Space for option values
} PAVP_ESC_GET_SET_OPTIONS;

//===========================================================================
// typedef:
//        PAVP_ESC_GET_SET_TEST_PARAMS
//
// Description:
//     This structure is for set/get of platform specific test parameters.
//---------------------------------------------------------------------------
typedef struct tagPAVP_ESC_GET_SET_TEST_PARAMS {
  ULONG TestOperation;  // Platform specific test operation
  ULONG Param[4];       // Space for parameters
} PAVP_ESC_GET_SET_TEST_PARAMS;

//===========================================================================
// typedef:
//        PAVP_ESC_QUERY_SESSION_STATUS_PARAMS
//
// Description:
//     The SessionType is input from the application passed down
//     to KMD determining what pool the session is allocated from.
//     The SessionIndex is the session slot to query. The session status,
//     instance count, and PAVP mode are returned from the database for
//     this slot.
//---------------------------------------------------------------------------
typedef struct tagPAVP_ESC_QUERY_SESSION_STATUS_PARAMS {
  PAVP_SESSION_ID PavpSessionId;     // in  - Session ID
  KM_PAVP_SESSION_TYPE SessionType;  // out - session type
  PAVP_SESSION_STATUS Status;        // out - session status
  ULONG InstanceCount;               // out - session instance count
  ULONG PavpMode;                    // out - session mode (lite, heavy, etc.)
  BOOLEAN bBlockNotify;              // in  - Block any event notifications
} PAVP_ESC_QUERY_SESSION_STATUS_PARAMS;

//===========================================================================
// typedef:
//        PAVP_ESC_GET_SET_SESSION_STATUS_PARAMS
//
// Description:
//     The SessionType is input from the application passed down
//     to KMD determining what pool the session is allocated from.
//     The SessionIndex is returned from the KMD if an allocation
//     is available. The session status is returned from the KMD
//      to indicate the status of this particular session (for WiDi use)
//---------------------------------------------------------------------------
typedef struct tagPAVP_ESC_GET_SET_SESSION_STATUS_PARAMS {
  PAVP_SESSION_ID PavpSessionId;       // in / out - App Type & Session Index
  KM_PAVP_SESSION_TYPE SessionType;    // in / out - session type
  KM_PAVP_SESSION_MODE SessionMode;    // in / out - session mode
  PAVP_SESSION_STATUS Status;          // in / out - session status
  ULONG InstanceCount;                 // out - session instance count
  PAVP_SESSION_STATUS PreviousStatus;  // out - previous session status
  PVOID CryptoSession;                 // in  - pointer to owning PavpDevice
} PAVP_ESC_GET_SET_SESSION_STATUS_PARAMS;

//===========================================================================
// typedef:
//        PAVP_ESC_GET_SET_SESSION_MODE_PARAMS
//
// Description:
//     The SessionType is input from the application passed down
//     to KMD determining what pool the session is allocated from.
//     The PavpMode is either PavpLite or PavpSerpent
//---------------------------------------------------------------------------
typedef struct tagPAVP_ESC_GET_SET_SESSION_MODE_PARAMS {
  PAVP_SESSION_ID PavpSessionId;     // in - App Type & Session Index
  KM_PAVP_SESSION_TYPE SessionType;  // out - session type
  ULONG PavpMode;  // in / out - session mode (lite, heavy, etc.)
} PAVP_ESC_GET_SET_SESSION_MODE_PARAMS;

//===========================================================================
// typedef:
//        PAVP_ESC_GET_SET_WIDI_STATUS_PARAMS
//
// Description:
//     The SessionType is input from the application passed down
//     to KMD determining what pool the session is allocated from.
//     The SessionIndex is returned from the KMD if an allocation
//     is available. The session status is returned from the KMD
//     to indicate the status of this particular session (for WiDi use)
//---------------------------------------------------------------------------
typedef struct tagPAVP_ESC_GET_SET_WIDI_STATUS_PARAMS {
  PAVP_SESSION_ID PavpSessionId;     // in - App Type & Session Index
  KM_PAVP_SESSION_TYPE SessionType;  // in / out - session type
  PAVP_SESSION_STATUS Status;        // out - session status
  ULONG streamCtr;                   // in / out - stream counter
  ULONGLONG rIV;                     // in / out - rIV vector
  ULONG WiDiEnabled;                 // in / out - WiDi enabled flag
  ULONG HDCPType;                    // in / out - HDCP session type
} PAVP_ESC_GET_SET_WIDI_STATUS_PARAMS;

//===========================================================================
// typedef:
//        PAVP_ESC_ACCESS_MMIO_REG_PARAMS
//
// Description:
//     This structure defined to retrieve PAVP MMIO register value.
//---------------------------------------------------------------------------
typedef struct tagPAVP_ESC_ACCESS_MMIO_REG_PARAMS {
  PAVP_GPU_REGISTER_OP PavpRegOp;
  PAVP_GPU_REGISTER_TYPE PavpMmioRegType;
  PAVP_GPU_REGISTER_VALUE PavpRegValue;
} PAVP_ESC_ACCESS_MMIO_REG_PARAMS;

//===========================================================================
// typedef:
//        PAVP_ESC_GET_CONNECTION_STATE_PARAMS
//
// Description:
//     Returns connection state and memory status state to UMD.
//---------------------------------------------------------------------------
typedef PAVP_GET_CONNECTION_STATE_PARAMS PAVP_ESC_GET_CONNECTION_STATE_PARAMS;

//===========================================================================
// typedef:
//        PAVP_ESC_GET_CONNECTION_STATE_PARAMS2
//
// Description:
//     Returns the Gen7+ connection state to the DDE (intended for Widi only).
//---------------------------------------------------------------------------
typedef PAVP_GET_CONNECTION_STATE_PARAMS2 PAVP_ESC_GET_CONNECTION_STATE_PARAMS2;

//===========================================================================
// typedef:
//        PAVP_ESC_GET_FRESHNESS_PARAMS
//
// Description:
//     This returns information to the UMD.  KMD should fill
//     this structure with a random freshness value returned
//     from the hardware.
//---------------------------------------------------------------------------
typedef struct tagPAVP_ESC_GET_FRESHNESS_PARAMS {
  PAVP_FRESHNESS_REQUEST_TYPE FreshnessType;
  DWORD Freshness[4];
} PAVP_ESC_GET_FRESHNESS_PARAMS;

//===========================================================================
// typedef:
//        PAVP_ESC_SET_FRESHNESS_PARAMS
//
// Description:
//     This sends info from UMD to KMD.
//---------------------------------------------------------------------------
typedef struct tagPAVP_ESC_SET_FRESHNESS_PARAMS {
  PAVP_FRESHNESS_REQUEST_TYPE FreshnessType;
} PAVP_ESC_SET_FRESHNESS_PARAMS;

//===========================================================================
// typedef:
//        PAVP_ESC_SET_WINDOW_POSITION_PARAMS
//
// Description:
//     Communicates the overlay position for the proprietary interface.
//---------------------------------------------------------------------------
typedef struct tagPAVP_ESC_SET_WINDOW_POSITION_PARAMS {
  RECT WindowPosition;
  RECT VisibleContent;
  HDC hdcMonitorId;
} PAVP_ESC_SET_WINDOW_POSITION_PARAMS;

//===========================================================================
// typedef:
//        PAVP_ESC_CONFIG_HW_KEY_EXCHANGE_PARAMS
//
// Description:
//     This structure passes information from UMD to KMD.
//     UMD should fill out the exchange mode member based on the
//     settings they want KMD to set.
//---------------------------------------------------------------------------
typedef struct tagPAVP_ESC_CONFIG_HW_KEY_EXCHANGE_PARAMS {
  BOOL bIsDaaKeyExchange;
  DWORD PchCommandId;  // BDW - To support PCH initiated commands submission to
                       // GPU over DMI channel
  PAVP_SESSION_ID SessionId;
} PAVP_ESC_CONFIG_HW_KEY_EXCHANGE_PARAMS;

//===========================================================================
// typedef:
//        PAVP_ESC_SET_PLANE_ENABLE_PARAMS
//
// Description:
//     These bits are input to be set prior to
//     displaying decrypted content.
//---------------------------------------------------------------------------
typedef struct tagPAVP_ESC_SET_PLANE_ENABLE_PARAMS {
  DWORD PlaneEnable[4];
  DWORD PlaneDisable[4];
  PAVP_PLANE_ENABLE_TYPE PlaneType;
  PAVP_SESSION_ID SessionId;
} PAVP_ESC_SET_PLANE_ENABLE_PARAMS;

//===========================================================================
// typedef:
//        PAVP_ESC_QUERY_SESSION_IN_PLAY
//
// Description:
//     Get info to determine if there is a PAVP still alive in hardware
//---------------------------------------------------------------------------
typedef struct tagPAVP_ESC_QUERY_SESSION_IN_PLAY {
  BOOL bSessionInPlay;
} PAVP_ESC_QUERY_SESSION_IN_PLAY;

//===========================================================================
// typedef:
//        PAVP_ESC_WIDI_SET_STREAM_KEY_PARAMS
//
// Description:
//     Set a new key using the Crypto Key Exchange command
//---------------------------------------------------------------------------
typedef struct tagPAVP_ESC_WIDI_SET_STREAM_KEY_PARAMS {
  KM_PAVP_SET_KEY_TYPE StreamType;
  DWORD EncryptedDecryptKey[4];
  DWORD EncryptedEncryptKey[4];
} PAVP_ESC_WIDI_SET_STREAM_KEY_PARAMS;

//===========================================================================
// typedef:
//        PAVP_ESC_WIDI_GET_WIDI_STATUS_PARAMS
//
// Description:
//     Get info to determine if WiDi is active.
//---------------------------------------------------------------------------
typedef struct tagPAVP_ESC_WIDI_GET_WIDI_STATUS_PARAMS {
  BOOLEAN bWiDiEnabled;
} PAVP_ESC_WIDI_GET_WIDI_STATUS_PARAMS;

//===========================================================================
// typedef:
//        PAVP_ESC_WIDI_SET_WIDI_STATUS_PARAMS
//
// Description:
//     This structure contains WiDi status which is called from DDE.
//---------------------------------------------------------------------------
typedef struct tagPAVP_ESC_WIDI_SET_WIDI_STATUS_PARAMS {
  BOOLEAN bWiDiEnabled;
  BOOLEAN bHDCP2Supported;
} PAVP_ESC_WIDI_SET_WIDI_STATUS_PARAMS;

//===========================================================================
// typedef:
//        PAVP_ESC_WIDI_QUERY_WIDI_STATUS
//
// Description:
//     This structure contains WiDi status which is called from DDE.
//---------------------------------------------------------------------------
// typedef struct tagPAVP_ESC_WIDI_QUERY_WIDI_STATUS
//{
//    UINT    WidiVersion;
//    UINT    HardwareID;
//    BOOL    bWidiDisplayActive;
//    UINT    NumActiveWirelessDisplays;
//} PAVP_ESC_WIDI_QUERY_WIDI_STATUS;

//===========================================================================
// typedef:
//        PAVP_ESC_QUERY_PAVP_USE_STATUS
//
// Description:
//     This structure defined to return IsPavpInUse. TRUE if PAVP session is in
//     use else FALSE
//---------------------------------------------------------------------------
typedef struct tagPAVP_ESC_QUERY_PAVP_USE_STATUS {
  PAVP_ESC_SYSTEM_PAVP_STATUS PavpMode;
  BOOL IsPavpInUse;
} PAVP_ESC_QUERY_PAVP_USE_STATUS;

//===========================================================================
// typedef:
//        _PAVP_ESC_MMIO_REG_PARAMS
//
// Description:
//     This structure defined to write/read PAVP MMIO register value.
//---------------------------------------------------------------------------
typedef struct _PAVP_ESC_MMIO_REG_PARAMS {
  PAVP_GPU_REGISTER_TYPE PavpMmioRegType;
  PAVP_GPU_REGISTER_VALUE PavpRegValue;
} PAVP_ESC_MMIO_REG_PARAMS;

typedef enum _PAVP_ESCAPE_ACTION {
  PAVP_ACTION_QUERY_CAPS,
  PAVP_ACTION_CONN_STATE,
  PAVP_ACTION_CONN_STATE2,
  PAVP_ACTION_GET_FRESHNESS,
  PAVP_ACTION_WINDOW_POSITION,
  PAVP_ACTION_CONFIG_HW_KEY_EXCHANGE,
  PAVP_ACTION_SET_PLANE_ENABLE,
  PAVP_ACTION_SET_NEW_KEY,
  PAVP_ACTION_QUERY_SESSION_STATUS,
  PAVP_ACTION_GET_SET_SESSION_STATUS,
  PAVP_ACTION_GET_WIDI_STATUS,
  PAVP_ACTION_SET_WIDI_STATUS,
  PAVP_ACTION_QUERY_SESSION_IN_PLAY,
  PAVP_ACTION_IS_PAVP_IN_USE,
  PAVP_ACTION_READ_MMIO_REG,
  PAVP_ACTION_WRITE_READ_MMIO_REG,
  PAVP_ACTION_ACCESS_MMIO_REG,
  PAVP_ACTION_GET_PAVP_MODE,
  PAVP_ACTION_GET_PARAMS,
  PAVP_ACTION_GET_SET_OPTIONS,
  PAVP_ACTION_TEST,
  PAVP_ACTION_COUNT
} PAVP_ESCAPE_ACTION;

//===========================================================================
// typedef:
//        PAVP_PROTECTION_INFO
//
// Description:
//     This describes the UMD KMD struture
//---------------------------------------------------------------------------

#pragma pack(4)
typedef struct _PAVP_PROTECTION_INFO {
  PAVP_ESCAPE_ACTION Action;
  ULONG ulProcessID;
  ULONG ulThreadID;
  BOOL bReturnValue;
  union {
    PAVP_ESC_QUERY_CAPS_PARAMS QueryCaps;
    PAVP_ESC_GET_PARAMS GetParams;
    PAVP_ESC_QUERY_SESSION_STATUS_PARAMS QuerySessionStatus;
    PAVP_ESC_GET_CONNECTION_STATE_PARAMS ConnState;
    PAVP_ESC_GET_CONNECTION_STATE_PARAMS2 ConnState2;
    PAVP_ESC_GET_FRESHNESS_PARAMS GetFreshness;
    PAVP_ESC_SET_FRESHNESS_PARAMS SetFreshness;
    PAVP_ESC_SET_WINDOW_POSITION_PARAMS WindowPosition;
    PAVP_ESC_CONFIG_HW_KEY_EXCHANGE_PARAMS ConfigHwKeyExchangeParams;
    PAVP_ESC_SET_PLANE_ENABLE_PARAMS SetPlaneEnableParams;
    PAVP_ESC_GET_SET_SESSION_STATUS_PARAMS GetSetSessionStatus;
    PAVP_ESC_GET_SET_SESSION_MODE_PARAMS GetSetSessionMode;
    PAVP_ESC_GET_SET_WIDI_STATUS_PARAMS GetSetWidiStatus;
    PAVP_ESC_ACCESS_MMIO_REG_PARAMS MmioRegOp;
    PAVP_ESC_QUERY_SESSION_IN_PLAY QuerySessionInPlay;
    PAVP_ESC_QUERY_PAVP_USE_STATUS QueryPavpUseStatus;
    PAVP_ESC_MMIO_REG_PARAMS MmioReg;
    PAVP_ESC_GET_SET_OPTIONS GetSetOptions;
    PAVP_ESC_GET_SET_TEST_PARAMS TestParams;
  };
} PAVP_PROTECTION_INFO;
#pragma pack()

//===========================================================================
// typedef:
//        PAVP_PROTECTION_ESCAPE
//
// Description:
//     This describes the UMD KMD struture
//---------------------------------------------------------------------------

typedef struct _PAVP_PROTECTION_ESCAPE {
  GFX_ESCAPE_HEADER_T Header;
  PAVP_PROTECTION_INFO PavpProtectInfo;
} PAVP_PROTECTION_ESCAPE;

//===========================================================================
// typedef:
//        GMM_PROTECTION_ESCAPE
//
// Description:
//     This describes the UMD KMD struture
//---------------------------------------------------------------------------
#pragma pack(4)
typedef struct {
  ULONG Action;  // GMM_ESCAPE_ACTION_T     Action;
  LONG NtStatus;
  ULONG RegType;  // GMM_ESCAPE_REG_TYPE     RegType;

  ULONG BusNumber;
  ULONG DevNumber;
  ULONG FunNumber;
  ULONG RegOffset;
  union {
    ULONG BytesToRead;
    ULONG BytesToWrite;
  };
  ULONGLONG RegValue;
} HDLESS_GMM_ESCAPE_REGISTER_VALUE;
#pragma pack()
/*
typedef enum
{
    GMM_ESCAPE_ACTION_GET_SEGMENT_INFO,     // return segment info
    GMM_ESCAPE_ACTION_GET_MEMORY_INFO,      // return memory info
    GMM_ESCAPE_ACTION_GET_REGISTERS,        // OBSOLETE
    GMM_ESCAPE_ACTION_DO_ALLOCATION,        // simulate allocation
    GMM_ESCAPE_ACTION_GET_MIPMAPOFFSETINFO, // Mipmap offsetInfo of allocation
    GMM_ESCAPE_ACTION_GET_OFFSET,           // OBSOLETE
    GMM_ESCAPE_ACTION_GET_SYSTEM_MEM,       // return Driver/OS Known Sys Mem
    GMM_ESCAPE_ACTION_READ_REGISTERS,       // return MMIO, MCH, PCI registers
    GMM_ESCAPE_ACTION_ODLAT,                // R/W Gen6+ performance registers
    GMM_ESCAPE_ACTION_WRITE_REGISTERS,      // write to MMIO, MCH, PCI registers
    GMM_ESCAPE_ACTION_UPDATE_CACHEABILITY,   // Update cacheability
    GMM_ESCAPE_ACTION_LOCK_NOTIFY,
    GMM_ESCAPE_ACTION_BANDWIDTH_CAPTURE,        // Bandwidth Capture Tool
    GMM_ESCAPE_ACTION_32BIT_ALIAS,              // Create/destroy 32-bit alias.
(OCL32 WA)
    GMM_ESCAPE_ACTION_GET_TRASH_PAGE_GFXADDR,   // Query gfx Address of the
Trash Page
    GMM_ESCAPE_ACTION_GET_HEAP32_GFXADDR,       // Query gfx Address of the
Heap32
    GMM_ESCAPE_ACTION_GET_SLM_ADDRESS_RANGE,    // Query OpenCL SLM gfx address
range
    GMM_ESCAPE_ACTION_READ_PAGE_TABLE,          // Read PTE
    GMM_ESCAPE_ACTION_CONFIGURE_ADDRESS_SPACE,  // Configure device address
space
    GMM_ESCAPE_ACTION_TILED_RESOURCE_OPERATION, // Create/Destroy/Map a
sparse/tiled resource
    GMM_ESCAPE_ACTION_SVM_DESTROY_ALLOC,        // Pre-declare alloc to be
"marked for death"
    GMM_ESCAPE_ACTION_SET_HW_PROTECTION_BIT,    // Set HW protection bit for
serpent encryption according to given value. GMM_BLOCKDESC for the surface will
be updated with the correct value of this bit.
    GMM_ESCAPE_ACTION_RES_UPDATE_AFTER_SHARED_OPEN, // Update ResInfo after a
shared resource is opened

} GMM_ESCAPE_ACTION_T;
*/
typedef struct {
  GFX_ESCAPE_HEADER_T Header;
  HDLESS_GMM_ESCAPE_REGISTER_VALUE PavpGmmInfo;
} LOCAL_GMM_ESCAPE_REGISTER_VALUE;

#endif  // __PAVP_ESCAPE_H
