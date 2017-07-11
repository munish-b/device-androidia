/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2014-2014
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

#ifndef __Hwcval_DrmEventThread_h__
#define __Hwcval_DrmEventThread_h__

#include "EventThread.h"
#include "HwcTestState.h"

#include <xf86drm.h>

#define HWCVAL_MAX_EVENTS 100

class DrmShimChecks;
class DrmShimCrtc;

//*****************************************************************************
//
// DrmShimEventHandler class - responsible for capturing and forwarding
// page flip and VBlank events
//
//*****************************************************************************

struct DrmEventData
{
    enum EventType
    {
        eVBlank,
        ePageFlip,
        eNone
    };

    EventType eventType;
    int fd;
    unsigned int seq;
    unsigned int sec;
    unsigned int usec;
    uint64_t data;
    DrmShimCrtc* crtc;

    DrmEventData()
      : eventType(DrmEventData::eNone),
        fd(0),
        seq(0),
        sec(0),
        usec(0),
        data(0),
        crtc(0)
    {
    }

    DrmEventData(const DrmEventData& rhs)
      : eventType(rhs.eventType),
        fd(rhs.fd),
        seq(rhs.seq),
        sec(rhs.sec),
        usec(rhs.usec),
        data(rhs.data),
        crtc(rhs.crtc)
    {
    }
};

class DrmShimEventHandler : public EventThread<DrmEventData, HWCVAL_MAX_EVENTS>, public HwcTestEventHandler
{
public:
    DrmShimEventHandler(DrmShimChecks* checks);
    virtual ~DrmShimEventHandler();

    void QueueCaptureVBlank(int fd, uint32_t crtcId);
    void CaptureVBlank(int fd, uint32_t crtcId);
    void CancelEvent(uint32_t crtcId);

    int WaitVBlank(drmVBlankPtr vbl);

    int HandleEvent(int fd, drmEventContextPtr evctx);

private:
    static DrmShimEventHandler* mInstance;
    DrmShimChecks* mChecks;

    drmEventContext mUserEvctx;
    drmEventContext mRealEvctx;

    int mDrmFd;
    DrmEventData mSavedPF;
    int64_t mSavedPFTime;

    virtual void onFirstRef();
    virtual bool threadLoop();

    bool RaiseEventFromQueue();
    void FwdVBlank(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data);
    void FwdPageFlip(int fd, unsigned int frame, unsigned int tv_sec, unsigned int tv_usec, void *data);
    virtual void Restore(uint32_t disp);

    static void vblank_handler(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data);
    static void page_flip_handler(int fd, unsigned int frame, unsigned int tv_sec, unsigned int tv_usec, void *data);

};


#endif // __Hwcval_DrmEventThread_h__
