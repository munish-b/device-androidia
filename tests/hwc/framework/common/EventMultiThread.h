/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2015
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

#ifndef __EventMultiThread_h__
#define __EventMultiThread_h__

#include "EventThread.h"
#include <utils/Thread.h>
#include "HwcTestUtil.h"

// Forward declaration
template<class C, int SIZE, int THREADS>
class EventMultiThread;

// Helper class for reading events from the event queue.
template<class C, int SIZE, int THREADS>
class EventReaderThread : public android::Thread
{
public:
    EventReaderThread();
    virtual ~EventReaderThread();
    void Start(const char* name, EventMultiThread<C, SIZE, THREADS>* controller);

    virtual void onFirstRef();
    virtual bool threadLoop();

private:
    EventMultiThread<C, SIZE, THREADS>* mController;
    android::String8 mName;
};



//*****************************************************************************
//
// EventMultiThread class - contains a queue on to which events can be pushed
// Events are then dispatched to one of several event threads using the Do function.
//
//*****************************************************************************

template<class C, int SIZE, int THREADS>
class EventMultiThread : public EventThread<C, SIZE>
{
public:
    static const int mReaderThreads = THREADS-1;
    EventMultiThread(const char* name="Unknown");
    virtual ~EventMultiThread();

    // Caller must override to provide the function that is called when there is work to do.
    virtual void Do(C& entry) = 0;

    // Abort
    void Stop();

    // Main thread function, though it doesn't have to do anything here
    virtual bool threadLoop();

private:
    android::String8 mName;
    typedef EventReaderThread<C, SIZE, THREADS> EvReader;
    EvReader mReaders[mReaderThreads];
};


// EventReaderThread implementation
template<class C, int SIZE, int THREADS>
EventReaderThread<C, SIZE, THREADS>::EventReaderThread()
{
}

template<class C, int SIZE, int THREADS>
EventReaderThread<C, SIZE, THREADS>::~EventReaderThread()
{
}

template<class C, int SIZE, int THREADS>
void EventReaderThread<C, SIZE, THREADS>::Start(const char* name, EventMultiThread<C, SIZE, THREADS>* controller)
{
    // Configure
    mName = name;
    mController = controller;

    // Start
    run(mName.string(), android::PRIORITY_URGENT_DISPLAY + android::PRIORITY_MORE_FAVORABLE);
}

template<class C, int SIZE, int THREADS>
void EventReaderThread<C, SIZE, THREADS>::onFirstRef()
{
}

template<class C, int SIZE, int THREADS>
bool EventReaderThread<C, SIZE, THREADS>::threadLoop()
{
    C event;
    while (mController->ReadWait(event))
    {
        mController->Do(event);
    }

    return false;
}



// EventMultiThread implementation
template<class C, int SIZE, int THREADS>
EventMultiThread<C, SIZE, THREADS>::EventMultiThread(const char* name)
  : EventThread<C,SIZE>::EventThread(name),
    mName(name)
{
    for (uint32_t i=0; i<mReaderThreads; ++i)
    {
        android::String8 threadName = android::String8::format("%s %d", mName.string(), i);
        mReaders[i].Start(threadName, this);
    }
}

template<class C, int SIZE, int THREADS>
EventMultiThread<C, SIZE, THREADS>::~EventMultiThread()
{
}

template<class C, int SIZE, int THREADS>
bool EventMultiThread<C, SIZE, THREADS>::threadLoop()
{
    C event;
    while (this->ReadWait(event))
    {
        Do(event);
    }

    return false;
}

template<class C, int SIZE, int THREADS>
void EventMultiThread<C, SIZE, THREADS>::Stop()
{
    HWCLOGD("EventMultiThread %s::Stop()", mName.string());
    for (uint32_t i=0; i<mReaderThreads; ++i)
    {
        mReaders[i].Stop();
    }

    EventThread<C,SIZE>::Stop();
}


#endif // __EventMultiThread_h__
