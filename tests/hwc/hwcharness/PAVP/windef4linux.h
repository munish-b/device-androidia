/*===================== begin_copyright_notice ==================================

INTEL CONFIDENTIAL
Copyright 2007-2014
Intel Corporation All Rights Reserved.

The source code contained or described herein and all documents related to the
source code ("Material") are owned by Intel Corporation or its suppliers or
licensors. Title to the Material remains with Intel Corporation or its suppliers
and licensors. The Material contains trade secrets and proprietary and confidential
information of Intel or its suppliers and licensors. The Material is protected by
worldwide copyright and trade secret laws and treaty provisions. No part of the
Material may be used, copied, reproduced, modified, published, uploaded, posted,
transmitted, distributed, or disclosed in any way without Intel's prior express
written permission.

No license under any patent, copyright, trade secret or other intellectual
property right is granted to or conferred upon you by disclosure or delivery
of the Materials, either expressly, by implication, inducement, estoppel or
otherwise. Any license under such intellectual property rights must be express
and approved by Intel in writing.

File Name: windef4linux.h

======================= end_copyright_notice ==================================*/
#ifndef WINDEF4LINUX_H
#define WINDEF4LINUX_H

#ifdef _WIN32
#error "This file is for Android / Linux and GHS Integrity only, and not to be included in a windows build"
#else

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h> //needed for ssize_t

//*************************************
// Windows data types
//*************************************
/*-------------------------------------------------------------------------------------
|   Size of C type in different platform
| Type        win32    win64    ubuntu32    ubuntu64    android-32    android-64
| bool          1        1         1            1            1            1
| char          1        1         1            1            1            1
| short         2        2         2            2            2            2
| short int     2        2         2            2            2            2
| int           4        4         4            4            4            4
| long          4        4         4            8            4            8
| long int      4        4         4            8            4            8
| long long     8        8         8            8            8            8
| float         4        4         4            4            4            4
| double        8        8         8            8            8            8
| long double   8        8         12           16           12           16
| void          -        -         1            1            1            1
| wchar_t       2        2         4            4            4            4
| void*         4        8         4            8            4            8
\------------------------------------------------------------------------------------*/

typedef void*               HANDLE;                                   //!< Opaque handle comprehended only by the OS
typedef void*               HINSTANCE;                                //!< Opaque handle comprehended only by the OS
typedef void*               HMODULE;                                  //!< Opaque handle comprehended only by the OS
typedef void**              PHANDLE;                                  //!< Pointer to a HANDLE

typedef void*               PTP_WAIT;                                 //!< New Thread Pool API
typedef void*               TP_WAIT_RESULT;                           //!< New Thread Pool API
typedef void*               TP_CALLBACK_INSTANCE;                     //!< New Thread Pool API
typedef void**              PTP_CALLBACK_INSTANCE;                    //!< New Thread Pool API

typedef void                VOID, *PVOID, *LPVOID;                    //!< Void
typedef const void          *LPCVOID;                                 //!< Const pointer to void
typedef void                *PCALLBACK_OBJECT;
typedef void*               HDC;

typedef char                INT8, *PINT8;                             //!< 8 bit signed value
typedef char                CHAR, *PCHAR, *LPSTR, TCHAR, *LPTSTR;     //!< 8 bit signed value
typedef const char          *PCSTR, *LPCSTR, *LPCTSTR;
typedef unsigned char       BYTE, *PBYTE, *LPBYTE;                    //!< 8 bit unsigned value
typedef unsigned char       UINT8, *PUINT8, UCHAR, *PUCHAR;           //!< 8 bit unsigned value

typedef int16_t             INT16, *PINT16, SHORT, *PSHORT;           //!< 16 bit signed value
typedef uint16_t            UINT16, *PUINT16, USHORT, *PUSHORT;       //!< 16 bit unsigned value
typedef uint16_t            WORD, *PWORD;                             //!< 16 bit unsigned value

typedef int                 NTSTATUS;
typedef int32_t             INT, *PINT, INT32, *PINT32;               //!< 32 bit signed value
typedef int32_t             HRESULT, LSTATUS;
typedef uint32_t            UINT, *PUINT, UINT32, *PUINT32;           //!< 32 bit unsigned value
typedef uint32_t            DWORD, *PDWORD, *LPDWORD;                 //!< 32 bit unsigned value
typedef int32_t             BOOL, *PBOOL;
typedef uint8_t             BOOLEAN;                                  //!< Boolean value - true >= 1 or false == 0
typedef uint32_t            HKEY;

typedef int32_t             LONG, *PLONG;                             //!< 32 bit unsigned value
typedef int64_t             INT64, *PINT64, LONGLONG, *PLONGLONG;     //!< 64 bit signed value
typedef int64_t             REFERENCE_TIME;
typedef uint32_t            ULONG, *PULONG;
typedef uint64_t            UINT64, *PUINT64, ULONGLONG;              //!< 64 bit unsigned value
typedef uint64_t            ULONG64;                                  //!< 64 bit unsigned value
typedef uint64_t            QWORD, *PQWORD;                           //!< 64 bit unsigned value

typedef float               FLOAT, *PFLOAT;                           //!< Floating point value
typedef double              DOUBLE;

typedef size_t              SIZE_T;                                   //!< unsigned size value
typedef ssize_t             SSIZE_T;                                  //!< signed size value

typedef uintptr_t           ULONG_PTR;                                //!< unsigned long type used for pointer precision
typedef intptr_t            INT_PTR;

typedef int                 errno_t;

typedef union _ULARGE_INTEGER
{
    struct
    {
        uint32_t                LowPart;
        uint32_t                HighPart;
    } u;

    struct
    {
        uint32_t                LowPart;
        uint32_t                HighPart;
    } ;

    uint64_t                    QuadPart;
} ULARGE_INTEGER;

typedef union _LARGE_INTEGER
{
    struct
    {
        int32_t LowPart;
        int32_t HighPart;
    } u;

    struct
    {
        int32_t LowPart;
        int32_t HighPart;
    } ;

    int64_t QuadPart;
} LARGE_INTEGER;

typedef LARGE_INTEGER* PLARGE_INTEGER;

typedef struct tagRECT
{
    int32_t    left;
    int32_t    top;
    int32_t    right;
    int32_t    bottom;
} RECT, *PRECT;

#define TRUE                    1
#define FALSE                   0

#define S_OK                    0
#define S_FALSE                 1
#define E_ABORT                 (0x80004004)
#define E_FAIL                  (0x80004005)
#define E_OUTOFMEMORY           (0x8007000E)
#define E_INVALIDARG            (0x80070057)

//*************************************
// Windows macros
//*************************************

#define IN
#define OUT
#define STDCALLTYPE

#define CONST const

#ifndef C_ASSERT
    #define __UNIQUENAME( a1, a2 )  __CONCAT( a1, a2 )
    #define UNIQUENAME( __text )    __UNIQUENAME( __text, __COUNTER__ )
#ifndef __ghs__
    #define C_ASSERT(e) typedef char UNIQUENAME(STATIC_ASSERT_)[(e)?1:-1]
#else
    #define C_ASSERT(e) //TODO ghs instr compilation fix
#endif
#endif

#define __S_INLINE              inline

//Linux use cdecl as default one
// __cdecl and __stdcall are deprecated for x86-64
#ifdef __x86_64__
    #define __stdcall
    #define __cdecl
#else
    #define __stdcall               __attribute__((__stdcall__))
    #define __cdecl                 __attribute__((__cdecl__))
#endif

#define PHYSICAL_ADDRESS        LARGE_INTEGER

#define VER_NT_WORKSTATION      0x0000001

typedef INT_PTR (*PROC)();
typedef INT_PTR (*FARPROC)();

//------------------------------------------------------------------------
// Result code definitions
//------------------------------------------------------------------------
#ifndef STATUS_SUCCESS
    #define STATUS_SUCCESS                  (0x0L)
    #define STATUS_UNSUCCESSFUL             (0xC0000001L)
    #define STATUS_NOT_SUPPORTED            (0xC00000BBL)
    #define STATUS_INVALID_PARAMETER        (0xC000000DL)
    #define STATUS_NOT_IMPLEMENTED          (0xC0000002L)
    #define STATUS_NO_SUCH_DEVICE           (0xC000000EL)
    #define STATUS_INVALID_DEVICE_REQUEST   (0xC0000010L)
    #define STATUS_BUFFER_TOO_SMALL         (0xC0000023L)
    #define STATUS_DEVICE_NOT_READY         (0xC00000A3L)
    #define STATUS_INVALID_OWNER            (0xC000005AL)
    #define STATUS_DATA_ERROR               (0xC000003EL)
#endif

// TODO. Find a better place.for these
typedef void                    *PHW_STATUS;
#define SIZE32(x)               ((uint32_t)(sizeof(x)/sizeof(uint32_t)))

#endif  // WIN32
#endif  // #ifndef WINDEF4LINUX_H

