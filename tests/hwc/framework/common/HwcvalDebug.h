/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2017
 * Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to the
 * source code ("Material") are owned by Intel Corporation or its suppliers or
 * licensors. Title to the Material remains with Intel Corporation or its suppliers
 * and licensors. The Material contains trade secrets and proprietary and confidential
 * information of Intel or its suppliers and licensors. The Material is protected by
 * worldwide copyright and trade secret laws and treaty provisions. No part of the
 * Material may be used, copied, reproduced, modified, published, uploaded, posted,
 * transmitted, distributed, or disclosed in any way without Intels prior express
 * written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel
 * or otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 *
 */

#ifndef INTEL_HWCVAL_DEBUG_H
#define INTEL_HWCVAL_DEBUG_H

#include <cutils/log.h>

#include <hardware/hwcomposer.h>
#include <utils/String8.h>

// Trace support
#include <utils/Trace.h>

// Mutex support
#include <utils/Mutex.h>
#include <utils/Condition.h>

namespace android
{
    class Mutex;
    class Condition;
};

namespace Hwcval {

#define MUTEX_CONDITION_DEBUG           0 // Debug mutex/conditions.

using namespace android;

// This ScopedTrace function compiles away properly when disabled. Android's one doesnt, it
// leaves strings and atrace calls in the code.
class HwcvalScopedTrace
{
public:
    inline HwcvalScopedTrace(bool bEnable, const char* name)
        : mbEnable(bEnable)
    {
        if (mbEnable)
            atrace_begin(ATRACE_TAG_GRAPHICS,name);
    }

    inline ~HwcvalScopedTrace()
    {
        if (mbEnable)
            atrace_end(ATRACE_TAG_GRAPHICS);
    }
private:
    bool mbEnable;
};

// Conditional variants of the macros in utils/Trace.h
#define ATRACE_INT_IF(enable, name, value)  do { if ( enable ) { ATRACE_INT( name, value ); } } while (0)
#define ATRACE_EVENT_IF(enable, name)       do { ATRACE_INT_IF( enable, name, 1 ); ATRACE_INT_IF( enable, name, 0 ); } while (0)

// Wrapper Mutex and Condition classes that add some debug and trap deadlocks.
class Mutex
{
    public:
        static const uint64_t mLongTime = 1000000000;  //< 1 second.
        static const uint32_t mSpinWait = 1000;        //< 1 millisecond.
        Mutex( );
        Mutex(const char* name);
        Mutex(int type, const char* name = NULL);
        ~Mutex( );
        int lock( );
        int unlock( );
        bool isHeld( void );
        void incWaiter( void );
        void decWaiter( void );
        uint32_t getWaiters( void );
        class Autolock
        {
            public:
                Autolock( Mutex& m );
                ~Autolock( );
            private:
                Mutex& mMutex;
        };

        // lock if possible; returns 0 on success, error otherwise
        status_t    tryLock();


    private:
        friend class Condition;
        bool mbInit:1;
        android::Mutex mMutex;
        pid_t   mTid;
        nsecs_t mAcqTime;
        uint32_t mWaiters;
};

class Condition
{
    public:
        Condition( );
        ~Condition( );
        int waitRelative( Mutex& mutex, nsecs_t timeout );
        int wait( Mutex& mutex );
        void signal( );
        void broadcast( );
    private:
        bool mbInit:1;
        uint32_t mWaiters;
        android::Condition mCondition;
};

}; // namespace Hwcval

#define INTEL_UFO_HWC_ASSERT_MUTEX_HELD( M ) ALOG_ASSERT( M.isHeld() );
#define INTEL_UFO_HWC_ASSERT_MUTEX_NOT_HELD( M ) ALOG_ASSERT( !M.isHeld() );

#endif // INTEL_UFO_HWC_DEBUG_H
