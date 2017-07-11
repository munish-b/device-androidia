/****************************************************************************
 *
 * Copyright (c) Intel Corporation (2015).
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
 * @file    WidiShimService.h
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    9th October 2014
 * @brief   Header for the Widi service shim.
 *
 * @details Top-level header for the Widi shim service. This service is started
 *          by HwcShim::HwcShimInit when the shims are initialised. Note, that
 *          the Widi shim registers its service as 'hwc.widi' and so must be
 *          running before the HWC is initialised. The HWC then detects the
 *          presence of the Widi shim when it goes to start its internal Widi
 *          service and so starts its service as 'hwc.widi.real'. This is then
 *          used by the Widi shim to do the back-end processing.
 *
 *****************************************************************************/

#ifndef __WidiShimService_h__
#define __WidiShimService_h__

#include "IFrameServer.h"
#include "HwcTestDefs.h"
#include "WidiShimFrameListener.h"
#include "WidiShimFrameTypeChangeListener.h"

class HwcTestState;
class HwcTestKernel;

class WidiShimService : public BnFrameServer
{

public:
    /* Class design - big 5 plus destructor */
    WidiShimService() = delete;
    WidiShimService(const WidiShimService& rhs) = delete;
    WidiShimService(WidiShimService&& rhs) = delete;
    WidiShimService& operator=(const WidiShimService& rhs) = delete;
    WidiShimService& operator=(WidiShimService&& rhs) = delete;
    virtual ~WidiShimService() = default;

    WidiShimService(android::sp<android::IServiceManager> sm, HwcTestState *testState)
        : mpSm(sm), mpTestState(testState) {}

    virtual android::status_t start(android::sp<IFrameTypeChangeListener> frameTypeChangeListener,
        bool disableExtVideoMode);
    virtual android::status_t stop(bool isConnected);
    virtual android::status_t setResolution(const FrameProcessingPolicy& policy,
        android::sp<IFrameListener> frameListener);

#if LIBHWCWIDI_USES_ORIGINAL_MCG_API
    virtual android::status_t notifyBufferReturned(int);
    virtual android::status_t notifyBufferReturned(buffer_handle_t handle);
#else
    virtual android::status_t notifyBufferReturned(int) { return android::BAD_VALUE; }
    virtual android::status_t notifyBufferReturned(buffer_handle_t) { return android::BAD_VALUE; }
#endif

    static void instantiate(HwcTestState *testState);

private:

    android::sp<IFrameServer> mpHwcWidiService;
    android::sp<android::IServiceManager> mpSm;

    android::sp<WidiShimFrameListener> mpFrameListener;
    android::sp<WidiShimFrameTypeChangeListener> mpFrameTypeChangeListener;

    HwcTestState *mpTestState = NULL;
    HwcTestKernel *mpTestKernel = NULL;

    // Variables for detecting the following errors:
    //
    // 1. Cycles of FrameTypeChange/SetResolution messages
    // 2. Excessive numbers of SetResolution messages
    int32_t mMaxSetResolutions = 1;
    int32_t mMaxSetResolutionWindow = 10;

    // Number of SetResolution messages received in this window
    int32_t mSetResolutionCount = 0;

    // Frame count for previous SetResolution message
    int32_t mFrameCountForPreviousSetResolution = 0;
};

#endif // __WidiShimService_h__
