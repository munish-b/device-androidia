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
* File Name:            HwcTestCompValThread.h
*
* Description:          Composition Validation thread class definition
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#ifndef __HwcTestCompValThread_h__
#define __HwcTestCompValThread_h__

#include <utils/Thread.h>
#include <utils/Condition.h>
#include "HwcTestState.h"
#include "HwcTestReferenceComposer.h"
#include "DrmShimBuffer.h"
#include "HwcTestDebug.h"

#include <hardware/hwcomposer_defs.h>
#include "HwcvalContent.h"

class HwcTestCompValThread : public android::Thread
{
public:
    HwcTestCompValThread();
    virtual ~HwcTestCompValThread();

    // Request reference composition of the given layer list
    // and store the result in the reference composition buffer attached to the DrmShimBuffer.
    bool Compose(android::sp<DrmShimBuffer> buf, Hwcval::LayerList& sources, Hwcval::ValLayer& dest);
    void Compare(android::sp<DrmShimBuffer> buf);
    void KillThread();

    bool IsBusy();
    void WaitUntilIdle();

    // Non-threaded
    void TakeCopy(android::sp<DrmShimBuffer> buf);
    void TakeTransformedCopy(const hwcval_layer_t *layer,
                             android::sp<DrmShimBuffer> buf, uint32_t width,
                             uint32_t height);
    android::sp<android::GraphicBuffer> CopyBuf(android::sp<DrmShimBuffer> buf);


private:
    //Thread functions
    virtual bool threadLoop();
    virtual android::status_t readyToRun();

    // In-thread local functions
    bool GetWork();
    void DoCompare();

    void ClearLocked(android::sp<DrmShimBuffer>& buf);
    void SkipComp(android::sp<DrmShimBuffer>& buf);
    void QueueFenceForClosure(int fence);

    // Composition data
    hwcval_layer_t mDest;

    // Buffer we will, are, or just have composed
    android::sp<DrmShimBuffer> mBuf;

    // Buffer we should compose
    android::sp<DrmShimBuffer> mBufToCompose;

    // Comparison data
    android::sp<DrmShimBuffer> mBufToCompare;

    // We don't need to compare the whole buffer: Just the part HWC was using as a composition target
    hwc_rect_t mRectToCompare;

    // Should we use alpha in the comparison (assuming the format supports it)
    bool mUseAlpha;

    // Thread management
    Hwcval::Condition mCondition;
    Hwcval::Mutex mMutex;

    volatile uint32_t mValSeq;     // Validation sequence

    volatile int mFenceForClosure;

    uint32_t mConsecutiveAbortedCompareCount;

    // The reference composition engine
    HwcTestReferenceComposer mComposer;
};

#endif // __HwcTestCompValThread_h__
