/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
**
** Copyright (c) Intel Corporation (2008).
**
** INTEL MAKES NO WARRANTY OF ANY KIND REGARDING THE CODE.  THIS CODE IS
** LICENSED ON AN "AS IS" BASIS AND INTEL WILL NOT PROVIDE ANY SUPPORT,
** ASSISTANCE, INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL DOES NOT
** PROVIDE ANY UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY
** DISCLAIMS ANY WARRANTY OF MERCHANTABILITY, NONINFRINGEMENT, FITNESS FOR ANY
** PARTICULAR PURPOSE, OR ANY OTHER WARRANTY.  Intel disclaims all liability,
** including liability for infringement of any proprietary rights, relating to
** use of the code. No license, express or implied, by estoppel or otherwise,
** to any intellectual property rights is granted herein.
**
**
** File Name  : pavp_types.h
**
** Abstract   : DXVA PAVP Device
**
** Environment: Windows Vista
**
** Notes      :
**
**+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef DXVA_PAVP_TYPES_INCLUDE
#define DXVA_PAVP_TYPES_INCLUDE

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

////////////////////////////////////////////////////////////////////////
// This file should be minimized to include only types used by both the
// KmRender component and the PAVP Escape interface.
// Keep in mind that anything added to this file is also seen in UMD.
////////////////////////////////////////////////////////////////////////

typedef enum {
  PAVP_FRESHNESS_REQUEST_DECRYPT = 0,
  PAVP_FRESHNESS_REQUEST_ENCRYPT = 1,
} PAVP_FRESHNESS_REQUEST_TYPE;

typedef enum {
  KM_PAVP_SESSION_TYPE_DECODE = 0,
  KM_PAVP_SESSION_TYPE_TRANSCODE = 1,
  KM_PAVP_SESSION_TYPE_WIDI = 2,
  KM_PAVP_SESSION_TYPES_MAX = 3
} KM_PAVP_SESSION_TYPE;

typedef enum {
  KM_PAVP_SET_KEY_DECRYPT = 0,  // Update Sn_d
  KM_PAVP_SET_KEY_ENCRYPT = 1,  // Update Sn_e
  KM_PAVP_SET_KEY_BOTH = 2,     // Update both Sn_d and Sn_e
  KM_PAVP_SET_KEY_FIXED_EXCHANGE =
      4,  // Reset to a new S1 (for fixed key exchange)
} KM_PAVP_SET_KEY_TYPE;

// The enum values of KM_PAVP_SESSION_MODE and GMM_PAVP_MODE (gmmCommonDefns.h)
// should match .
// Android KMD (CoreU) will use KM_PAVP_SESSION_MODE
// Windows KMD will use GMM_PAVP_MODE. Eventually, Win KMD should migrate to
// KM_PAVP_SESSION_MODE
typedef enum {
  KM_PAVP_SESSION_MODE_UNKNOWN = 0,
  KM_PAVP_SESSION_MODE_BIG_PCM = 1,      // Big PCM Mode
  KM_PAVP_SESSION_MODE_LITE = 2,         // Lite Mode
  KM_PAVP_SESSION_MODE_HEAVY = 3,        // Heavy Mode
  KM_PAVP_SESSION_MODE_PER_APP_MEM = 4,  // Per App Mode
  KM_PAVP_SESSION_MODE_ISO_DEC = 5,      // Isolated Decode Mode
  KM_PAVP_SESSION_MODE_STOUT = 6,      // Isolated Decode with Authenticated EU
  KM_PAVP_SESSION_MODE_THV_D = 7,      // Isolated Display/Threadville-Display
  KM_PAVP_SESSION_MODE_HUC_GUARD = 8,  // HuC Signed cmd buff
  KM_PAVP_SESSION_MODES_MAX = 9
} KM_PAVP_SESSION_MODE;

// PAVP display enable types
typedef enum tagPAVP_PLANE_ENABLE_TYPE {
  PAVP_PLANE_DISPLAY_C = 0x0080,
  PAVP_PLANE_SPRITE_C = 0x0100,
  PAVP_PLANE_CURSOR_C = 0x0200,
  PAVP_PLANE_DISPLAY_A = 0x0400,
  PAVP_PLANE_DISPLAY_B = 0x0800,
  PAVP_PLANE_SPRITE_A = 0x1000,
  PAVP_PLANE_SPRITE_B = 0x2000,
  PAVP_PLANE_CURSOR_A = 0x4000,
  PAVP_PLANE_CURSOR_B = 0x8000
} PAVP_PLANE_ENABLE_TYPE;

// PAVP session status
typedef enum tagPAVP_CRYPTO_SESSION_STATUS {
  PAVP_CRYPTO_SESSION_STATUS_OK = 0x0,
  PAVP_CRYPTO_SESSION_STATUS_KEY_LOST = 0x1,
  PAVP_CRYPTO_SESSION_STATUS_KEY_AND_CONTENT_LOST = 0x2
} PAVP_CRYPTO_SESSION_STATUS;

// PAVP key programming status
typedef enum tagPAVP_GET_SET_DATA_NEW_HW_KEY {
  PAVP_GET_SET_HW_KEY_NONE =
      0x0,  // Return App ID from UMD without intreaction with GPU and ME
  PAVP_SET_CEK_KEY_IN_ME = 0x1,        // Program CEK in ME
  PAVP_GET_CEK_KEY_FROM_ME = 0x2,      // Obtain CEK from ME
  PAVP_SET_CEK_KEY_IN_GPU = 0x3,       // Program key rotation blob on GPU
  PAVP_SET_CEK_INVALIDATE_IN_ME = 0x4  // Cancel CEK in ME
} PAVP_GET_SET_DATA_NEW_HW_KEY;

typedef struct tagPAVP_SESSION_ID {
  union {
    struct {
      unsigned char SessionIndex : 7;  // PAVP Session Index for this App ID
      unsigned char SessionType : 1;   // PAVP HW App ID
    };
    unsigned char SessionIdValue;
  };
} PAVP_SESSION_ID;

typedef struct tagPAVP_SESSION_ASSOCIATORS {
  void *pContext;
  void *pCryptoSession;
} PAVP_SESSION_ASSOCIATORS;

static const PAVP_SESSION_ASSOCIATORS
    PAVP_SESSION_ASSOCIATORS_INITIALIZER_INVALID = {
        NULL,  // pContext
        NULL   // pCryptoSession
};

// These are states a PAVP session goes through to become established or torn
// down.
// Refer to the pcp_pavp_session_states document in the content protection share
// for more information
typedef enum tagPAVP_SESSION_STATUS {
  PAVP_SESSION_GET_ANY_DECODE_SESSION_IN_USE =
      0,  // Input to check if any decode session is in use
  PAVP_SESSION_GET_SESSION_STATUS,  // Input to request the status of the
                                    // current session
  PAVP_SESSION_UNSUPPORTED,  // Can't be used, session slots are marked with
                             // this at boot
  PAVP_SESSION_POWERUP,  // Driver has initialized (powered up), UMD can setup
                         // for PAVP
  PAVP_SESSION_POWERUP_IN_PROGRESS,  // UMD is in process of running power up
                                     // tasks, indicates to other processes this
                                     // is in progress
  PAVP_SESSION_UNINITIALIZED,        // Available, but init sequence required
  PAVP_SESSION_IN_INIT,  // In initialization process, slot is reserved by UMD
                         // process
  PAVP_SESSION_INITIALIZED,  // Init sequence complete (or not required) but key
                             // exchange still pending
  PAVP_SESSION_READY,  // Key exchange and plane enables all complete, ready for
                       // decode
  PAVP_SESSION_TERMINATED,  // Indicates this session slot's UMD process has
                            // crashed, will be recovered by UMD when reserved.
  PAVP_SESSION_TERMINATE_DECODE,       // Called from UMD Pavp_Destroy
  PAVP_SESSION_TERMINATE_WIDI,         // Called with the Widi app is closing
  PAVP_SESSION_TERMINATE_IN_PROGRESS,  // Called when a session is about to be
                                       // destroyed gracefully by the UMD
                                       // process
  PAVP_SESSION_IN_USE,                 // Returned when all sessions are in use
  PAVP_SESSION_ATTACKED,  // Session has been attacked via autoteardown
  PAVP_SESSION_ATTACKED_IN_PROGRESS,  // UMD is recovering from attacked state,
                                      // indicates to other processes this is in
                                      // progress
  PAVP_SESSION_RECOVER,  // We have recovered from powerup or attack, puts
                         // sessions in UNINITIALIZED state, ready for use
  PAVP_SESSION_RECOVER_IN_PROGRESS,  // Called when UMD is about to be recover
                                     // from attack.
  PAVP_SESSION_POWERUPINIT,  // WiDi only - WiDi needs to do power up init from
                             // the KMD via this status
  PAVP_SESSION_POWERUP_RESERVED,  // Session has been attacked, but had been
                                  // reserved when this happened
  PAVP_SESSION_POWERUP_RESERVED_IN_PROGRESS,  // This is a powerup reserved
                                              // session whose recovery is
                                              // handled by some process
  PAVP_SESSION_POWERUP_RESERVED_RECOVERED,    // powerup recovery has completed
                                              // and this session is awaiting
                                              // uninitialization by its owner
  PAVP_SESSION_ATTACKED_RESERVED,  // Session has been attacked, but had been
                                   // reserved when this happened
  PAVP_SESSION_ATTACKED_RESERVED_IN_PROGRESS,  // This is an attacked reserved
                                               // session whose recovery is
                                               // handled by some process
  PAVP_SESSION_ATTACKED_RESERVED_RECOVERED,    // Attack recovery has completed
                                               // and this session is awaiting
                                               // uninitialization by its owner
  PAVP_SESSION_UNKNOWN = 0xFFFFFFFF,  // Indicates unknown or meaningless state
} PAVP_SESSION_STATUS;

// This function is implemented in cp_pavpdevice.cpp.
extern const char *SessionStatus2Str(PAVP_SESSION_STATUS Status);

typedef struct tagPAVP_GET_CONNECTION_STATE_PARAMS {
  uint32_t Nonce;
  uint32_t ProtectedMemoryStatus[4];
  uint32_t PortStatusType;
  uint32_t DisplayPortStatus[4];
} PAVP_GET_CONNECTION_STATE_PARAMS;

typedef struct tagPAVP_GET_CONNECTION_STATE_PARAMS2 {
  uint32_t NonceIn;
  uint8_t ConnectionStatus[32];
  uint8_t ProtectedMemoryStatus[16];
} PAVP_GET_CONNECTION_STATE_PARAMS2;

typedef enum tagPAVP_GPU_REGISTER_OP {
  PAVP_REGISTER_READ = 1,       // Read Register
  PAVP_REGISTER_WRITE,          // Write Register
  PAVP_REGISTER_WRITEREAD,      // Write Register, followed by read
  PAVP_REGISTER_READWRITE_AND,  // Read, AND value, write back out
  PAVP_REGISTER_READWRITE_OR,   // Read, OR value, write back out
  PAVP_REGISTER_READWRITE_XOR,  // Read, XOR value, write back out
} PAVP_GPU_REGISTER_OP;

// do not but conditional compiles in this as it must be common between UMD,
// KMD, Windows Android etc
typedef enum tagPAVP_GPU_REGISTER_TYPE {
  PAVP_REGISTER_INVALID =
      0,                //!< make 0 invalid as its common to make this mistake
  PAVP_REGISTER_APPID,  //!< The App ID Register
  PAVP_REGISTER_SESSION_IN_PLAY,  //!< The Session in Play Register
  PAVP_REGISTER_STATUS,  //!< The status register (for most recently active
                         //session)
  PAVP_REGISTER_DEBUG,   //!< The silicon debug register (PCH pairing status,
                         //attack log, etc
  PAVP_REGISTER_CONN_STATE_WRITE,  //!< Where to write a connection state value
                                   //(pre-silicon)
  PAVP_SIM_DELIVER_SESSION_KEY,    //!< Where to write a new (Kb)KF1 to use for
                                   //the session (pre-silicon)
  PAVP_SIM_WIDI_SESSION_KEY,  //!< Where to write a new (Kb)KF1 to use for the
                              //widi session (pre-silicon)
  PAVP_REGISTER_PAVPC,  //!< The silicon PAVP Configuration register (should be
                        //locked by BIOS)
  PAVP_MFX_MODE_REGISTER,
  PAVP_CONTROL_REGISTER,
  PAVP_REGISTER_GLOBAL_TERMINATE,
  PAVP_REGISTER_VLV_GUNIT_DISPLAY_GCI_CONTROL_REG,
  PAVP_SECURE_OFFSET_REG,  //!< Secure offset register used for signalling CB2
                           //insertion into ring
  PAVP_REGISTER_EXPLICIT_TERMINATE
} PAVP_GPU_REGISTER_TYPE;

typedef enum tagPAVP_GFX_ADDRESS_PATCHS {
  PAVP_GFX_ADDRESS_MULTIPATCHS = -1,  // Multiple patch locations
  PAVP_GFX_ADDRESS_SINGLEPATCH = 0    // Single patch location
} PAVP_GFX_ADDRESS_PATCHS;

// bit field definitions for the above registers
#define PAVP_REGISTER_GLOBAL_TERMINATE_BIT_DECODE 0x1
#define PAVP_REGISTER_EXPLICIT_TERMINATE_DECODE_MASK 0x4
#define PAVP_REGISTER_DEBUG_BIT_OMAC_VERIFICATION 0x8000
#define PAVP_REGISTER_SESSION_IN_PLAY_DECODE_SESSION_MASK 0x0000FFFF
// Mask to check if Decoder Session 15 is alive or not
#define PAVP_REGISTER_SESSION_IN_PLAY_ARBITRATOR_SESSION_MASK 0x8000

#define PAVP_INVALID_SESSION_ID 0xFF

#define PAVP_REGISTER_PAVPC_PAVPLOCK 0x4
#define PAVP_REGISTER_PAVPC_PAVPE 0x2  // enable
#define PAVP_REGISTER_PAVPC_PCME \
  0x1  // PAVP:[0]--Protected Content Memory Enable
#define PAVP_REGISTER_PAVPC_DEFAULT \
  0xAF900007  // from IVB WIn BIOS, use at own risk
#define PAVP_SECURE_OFFSET_DEF_VAL 0x000C0000  // from VLV
#define PAVP_ROTATION_KEY_BLOB_SIZE 0x10       // AES key blob size - 16 bytes

typedef unsigned long PAVP_GPU_REGISTER;
typedef unsigned long long PAVP_GPU_REGISTER_VALUE;

#endif  // DXVA_PAVP_TYPES_INCLUDE
