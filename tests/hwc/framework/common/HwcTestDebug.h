/****************************************************************************

Copyright (c) Intel Corporation (2014).

DISCLAIMER OF WARRANTY
NEITHER INTEL NOR ITS SUPPLIERS MAKE ANY REPRESENTATION OR WARRANTY OR
CONDITION OF ANY KIND WHETHER EXPRESS OR IMPLIED (EITHER IN FACT OR BY
OPERATION OF LAW) WITH RESPECT TO THE SOURCE CODE.  INTEL AND ITS SUPPLIERS
EXPRESSLY DISCLAIM ALL WARRANTIES OR CONDITIONS OF MERCHANTABILITY OR
FITNESS FOR A PARTICULAR PURPOSE.  INTEL AND ITS SUPPLIERS DO NOT WARRANT
THAT THE SOURCE CODE IS ERROR-FREE OR THAT OPERATION OF THE SOURCE CODE WILL
BE SECURE OR UNINTERRUPTED AND HEREBY DISCLAIM ANY AND ALL LIABILITY ON
ACCOUNT THEREOF.  THERE IS ALSO NO IMPLIED WARRANTY OF NON-INFRINGEMENT.
SOURCE CODE IS LICENSED TO LICENSEE ON AN "AS IS" BASIS AND NEITHER INTEL
NOR ITS SUPPLIERS WILL PROVIDE ANY SUPPORT, ASSISTANCE, INSTALLATION,
TRAINING OR OTHER SERVICES.  INTEL AND ITS SUPPLIERS WILL NOT PROVIDE ANY
UPDATES, ENHANCEMENTS OR EXTENSIONS.

File Name:      HwcTestDebug.h

Description:    Miscellaneous debug functions.

Environment:

Notes:

****************************************************************************/

#ifndef __HwcTestDebug_h__
#define __HwcTestDebug_h__

#include "HwcTestDefs.h"
#include "HwcTestState.h"
#include "DrmShimBuffer.h"

enum
{
    DUMP_BUFFER_TO_RAW = (1<<0),
    DUMP_BUFFER_TO_TGA = (1<<1)
};

bool HwcTestDumpGrallocBufferToDisk( const char* pchFilename,
                                     uint32_t num,
                                     buffer_handle_t grallocHandle,
                                     uint32_t outputDumpMask );

bool HwcTestDumpAuxBufferToDisk( const char* pchFilename,
                                 uint32_t num,
                                 buffer_handle_t grallocHandle );

bool HwcTestDumpMemBufferToDisk( const char* pchFilename,
                                 uint32_t num,
                                 const void* handle,
                                 Hwcval::buffer_details_t& bufferInfo,
                                 uint32_t stride,
                                 uint32_t outputDumpMask,
                                 uint8_t* pData );

#if HWCVAL_LOCK_DEBUG || defined(HWCVAL_LOCK_TRACE)
#include <utils/Mutex.h>
class HwcvalLock
{
    public:
        HwcvalLock(const char* funcName, const char* mutexName, Hwcval::Mutex& mutex) : mLock(mutex)
#ifdef HWCVAL_LOCK_TRACE
          , mTracer(ATRACE_TAG, funcName)
#endif
        {
            HWCLOGD_IF(HWCVAL_LOCK_DEBUG, "Thread %d Request lock mutex %s @ %p : %s", gettid(), mutexName, &mutex, funcName);
            mLock.lock();
            HWCLOGD_IF(HWCVAL_LOCK_DEBUG, "Thread %d Gained lock mutex %s @ %p : %s", gettid(), mutexName, &mutex, funcName);
        }

        HwcvalLock(const char* funcName, const char* mutexName, Hwcval::Mutex* mutex) : mLock(*mutex)
#ifdef HWCVAL_LOCK_TRACE
          , mTracer(ATRACE_TAG, funcName)
#endif
        {
            HWCLOGD_IF(HWCVAL_LOCK_DEBUG, "Thread %d Request lock mutex %s @ %p : %s", gettid(), mutexName, mutex, funcName);
            mLock.lock();
            HWCLOGD_IF(HWCVAL_LOCK_DEBUG, "Thread %d Gained lock mutex %s @ %p : %s", gettid(), mutexName, mutex, funcName);
        }

        ~HwcvalLock()
        {
            HWCLOGD_IF(HWCVAL_LOCK_DEBUG, "Thread %d Unlocking mutex @ %p", gettid(), &mLock);
            mLock.unlock();
        }

    private:
        Hwcval::Mutex& mLock;
#ifdef HWCVAL_LOCK_TRACE
        android::ScopedTrace mTracer;
#endif
};

#define HWCVAL_LOCK(L,M) char L##name[1024]; strcpy(L##name, __PRETTY_FUNCTION__); strcat(L##name,"-Mtx"); HwcvalLock L(L##name, #M, M)

#else
#define HWCVAL_LOCK(L,M) Hwcval::Mutex::Autolock L(M)
#endif


#endif // __HwcTestDebug_h__
