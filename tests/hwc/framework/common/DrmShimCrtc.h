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

File Name:      DrmShimCrtc.h

Description:    Class definition for DRMShimCrtc class.

Environment:

Notes:

****************************************************************************/
#ifndef __DrmShimCrtc_h__
#define __DrmShimCrtc_h__

#include "HwcTestCrtc.h"
#include <ui/GraphicBuffer.h>
#include "GrallocClient.h"

class DrmShimPlane;
class HwcTestKernel;
class HwcTestDisplayContents;

class DrmShimCrtc : public HwcTestCrtc
{
public:

    //-----------------------------------------------------------------------------
    // Constructor & Destructor
    DrmShimCrtc(uint32_t crtcId, uint32_t width, uint32_t height, uint32_t clock, uint32_t vrefresh);

    virtual ~DrmShimCrtc();

    void SetChecks(DrmShimChecks* checks);

    // Pipe index
    void SetPipeIndex(uint32_t pipeIx);
    uint32_t GetPipeIndex();

    // Connector Id
    void SetConnector(uint32_t connectorId);
    uint32_t GetConnector();
    virtual bool IsDRRSEnabled();

    // VBlank interception
    drmVBlankPtr SetupVBlank();
    drmVBlankPtr GetVBlank();
    void SetUserVBlank(drmVBlankPtr vbl);
    bool IssueVBlank(unsigned int frame, unsigned int sec, unsigned int usec, void *& userData);

    void SavePageFlipUserData(uint64_t userData);
    uint64_t GetPageFlipUserData();

    // Drm call duration evaluation
    void DrmCallStart();
    int64_t GetDrmCallDuration();
    int64_t GetTimeSinceVBlank();

    bool IsVBlankRequested(uint32_t frame);
    void* GetVBlankUserData();

    // Hotplug
    bool SimulateHotPlug(bool connected);

    // Latch power state
    uint32_t SetDisplayEnter(bool suspended);
    void StopSetDisplayWatchdog();
    bool WasSuspended();

    // Report power at start of set display and now
    const char* ReportSetDisplayPower(char* strbuf, uint32_t len = HWCVAL_DEFAULT_STRLEN);

    // Nuclear->SetDisplay conversion
    typedef int (*DrmModeAddFB2Func)(   int fd, uint32_t width, uint32_t height,
                                        uint32_t pixel_format,
                                        uint32_t bo_handles[4],
                                        uint32_t pitches[4],
                                        uint32_t offsets[4],
                                        uint32_t *buf_id, uint32_t flags);


    uint32_t GetBlankingFb(::intel::ufo::gralloc::GrallocClient& gralloc, DrmModeAddFB2Func addFb2Func, int fd);

private:
    // DRM checks
    DrmShimChecks* mChecks;

    // DRM pipe index
    uint32_t mPipeIx;

    // DRM connector id
    uint32_t mConnectorId;

    // Vblank structure issued to Drm
    drmVBlank mVBlank;

    // Vblank callback request data
    uint32_t mVBlankFrame;
    uint64_t mVBlankSignal;

    // User data from page flip event
    uint64_t mPageFlipUserData;

    // Start time for atomic DRM call
    uint64_t mDrmCallStartTime;

    // Power state at start of set display
    PowerState mPowerStartSetDisplay;
    bool mSuspendStartSetDisplay;

    // Blanking buffer
    android::sp<android::GraphicBuffer> mBlankingBuffer;
    uint32_t mBlankingFb;
};

inline void DrmShimCrtc::SetChecks(DrmShimChecks* checks)
{
    mChecks = checks;
}

inline bool DrmShimCrtc::WasSuspended()
{
    return mSuspendStartSetDisplay;
}

inline void DrmShimCrtc::SetPipeIndex(uint32_t pipeIx)
{
    mPipeIx = pipeIx;
}

inline uint32_t DrmShimCrtc::GetPipeIndex()
{
    return mPipeIx;
}

// Connector Id
inline void DrmShimCrtc::SetConnector(uint32_t connectorId)
{
    mConnectorId = connectorId;
}

inline uint32_t DrmShimCrtc::GetConnector()
{
    return mConnectorId;
}

#endif // __DrmShimCrtc_h__
