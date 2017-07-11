/****************************************************************************

Copyright (c) Intel Corporation (2015).

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

File Name:      HwcvalThreadTable.cpp

Description:    Hash table of thread states so we can report state of all threads
                in case of hang.

Environment:

Notes:

****************************************************************************/

#include "HwcvalThreadTable.h"
#include "HwcTestDefs.h"
#include "HwcTestState.h"

// Thread state hash table
static uint32_t mTids[HWCVAL_THREAD_TABLE_SIZE];
static const char* mThreadState[HWCVAL_THREAD_TABLE_SIZE];

void Hwcval::InitThreadStates()
{
    // Do not use logging in this function, or we will recurse
    for (uint32_t i=0; i<HWCVAL_THREAD_TABLE_SIZE; ++i)
    {
        mTids[i] = 0;
        mThreadState[i] = "";
    }
}

const char* Hwcval::SetThreadState(const char* str)
{
    uint32_t tid = gettid();
    uint32_t hash = tid % HWCVAL_THREAD_TABLE_SIZE;

    // Find the thread in the table
    uint32_t i = hash;
    do
    {
        if (tid == mTids[i])
        {
            const char* oldState = mThreadState[i];
            mThreadState[i] = str;
            return oldState;
        }

        if (++i >= HWCVAL_THREAD_TABLE_SIZE)
        {
            i = 0;
        }
    } while (i != hash);

    // Thread not found, so add it to the table
    i = hash;
    do
    {
        if (mTids[i] == 0)
        {
            mTids[i] = tid;
            mThreadState[i] = str;
            return "";
        }

        if (++i >= HWCVAL_THREAD_TABLE_SIZE)
        {
            i = 0;
        }
    } while (i != hash);

    HWCLOGI("Thread table full.");
    return "";
}

void Hwcval::ReportThreadStates()
{
    HWCLOGD("ReportThreadStates");
    for (uint32_t i=0; i<HWCVAL_THREAD_TABLE_SIZE; ++i)
    {
        if (mTids[i])
        {
            HWCLOGA("Thread %d: %s", mTids[i], mThreadState[i]);
        }
    }
}

Hwcval::PushThreadState::PushThreadState(const char* threadState)
{
    mOldThreadState = SetThreadState(threadState);
}

Hwcval::PushThreadState::~PushThreadState()
{
    SetThreadState(mOldThreadState);
}

