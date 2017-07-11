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

File Name:      DrmShimChecks.cpp

Description:    Drm shim implementation for check functions.

                See hwc_shim/hwc_shim.h for a full description of how the shims
                work

Environment:

Notes:

****************************************************************************/

#include <unistd.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <math.h>
#include <ufo/graphics.h>
#include "DrmShimChecks.h"
#include "DrmShimCrtc.h"
#include "DrmShimBuffer.h"
#include "BufferObject.h"
#include "HwcvalGeom.h"
#include "DrmShimWork.h"
#include "HwcTestState.h"
#include "HwcTestConfig.h"
#include "HwcTestDebug.h"
#include "HwcTestUtil.h"
#include "HwcvalPropertyManager.h"
#include "HwcvalThreadTable.h"

#include "DrmAtomic.h"

#include "GrallocClient.h"
#include "drm_fourcc.h"
#ifdef HWCVAL_TARGET_HAS_MULTIPLE_DISPLAY
#include "MultiDisplayType.h"
#endif

#undef LOG_TAG
#define LOG_TAG "DRM_SHIM"

using namespace Hwcval;

/// Broxton plane transform values
#define HWCVAL_DRM_ROTATE_0 (1<<0)
#define HWCVAL_DRM_ROTATE_90 (1<<1)
#define HWCVAL_DRM_ROTATE_180 (1<<2)
#define HWCVAL_DRM_ROTATE_270 (1<<3)
#define HWCVAL_DRM_REFLECT_X (1<<4)
#define HWCVAL_DRM_REFLECT_Y (1<<5)

/// Statistics declarations
static Hwcval::Statistics::Counter sHwPlaneTransformUsedCounter("hw_plane_transforms_used");
static Hwcval::Statistics::Counter sHwPlaneScaleUsedCounter("hw_plane_scalers_used");


/// Constructor
DrmShimChecks::DrmShimChecks()
  : HwcTestKernel(),
    mShimDrmFd(0),
    mCrtcs((DrmShimCrtc*) 0),
    mPropMgr(0),
    mUniversalPlanes(false),
    mDrmFrameNo(0),
    mDrmParser(this, mProtChecker, &mLogParser)
{
    for (uint32_t i=0; i<HWCVAL_MAX_CRTCS; ++i)
    {
        mCurrentFrame[i] = -1;
        mLastFrameWasDropped[i] = false;
    }

    memset(mSetDisplay, 0, sizeof(drm_mode_set_display)* HWCVAL_MAX_CRTCS);
}

/// Destructor
DrmShimChecks::~DrmShimChecks()
{
    HWCLOGI("Destroying DrmShimChecks");

    {
        HWCVAL_LOCK(_l,mMutex);
        mWorkQueue.Process();
    }

}

void DrmShimChecks::CheckGetResourcesExit(int fd, drmModeResPtr res)
{
    HWCVAL_UNUSED(fd);

    if (res)
    {
        ALOG_ASSERT(res->count_crtcs <= HWCVAL_MAX_CRTCS);

        for (int i=0; i<res->count_crtcs; ++i)
        {
            DrmShimCrtc* crtc = CreatePipe(i, res->crtcs[i]);
            mCrtcs.replaceValueFor(crtc->GetCrtcId(), crtc);
        }
    }
}

void DrmShimChecks::OverrideDefaultMode(drmModeConnectorPtr pConn)
{
    uint32_t maxScore = 0;
    int realPreferredMode = -1;
    int newPreferredMode = -1;

    for (int i=0; i<pConn->count_modes; ++i)
    {
        drmModeModeInfoPtr pMode = pConn->modes+i;
        uint32_t score = 0;

        if (pMode->hdisplay == mPrefHdmiWidth)
        {
            ++score;
        }

        if (pMode->vdisplay == mPrefHdmiHeight)
        {
            ++score;
        }

        if (pMode->vrefresh == mPrefHdmiRefresh)
        {
            ++score;
        }

        if (score > maxScore)
        {
            newPreferredMode = i;
            maxScore = score;
        }

        if (pMode->type & DRM_MODE_TYPE_PREFERRED)
        {
            realPreferredMode = i;
        }
    }

    if (maxScore == 0)
    {
        HWCLOGI("No mode matching preferred mode override found.");
    }
    else if (realPreferredMode != newPreferredMode)
    {
        // unset preferred mode on mode actually preferred by monitor
        pConn->modes[realPreferredMode].type &= ~DRM_MODE_TYPE_PREFERRED;

        // set preferred mode on the one we want
        drmModeModeInfoPtr prefMode = pConn->modes + newPreferredMode;
        prefMode->type |= DRM_MODE_TYPE_PREFERRED;

        if (maxScore == 3)
        {
            HWCLOGI("Exact match with preferred mode override:");
        }
        else
        {
            HWCLOGI("Closest match with preferred mode override:");
        }

        HWCLOGI("Mode %d %dx%d refresh=%d", newPreferredMode,
            prefMode->hdisplay, prefMode->vdisplay, prefMode->vrefresh);
    }
}

void DrmShimChecks::RandomizeModes(int& count, drmModeModeInfoPtr& modes)
{
    bool modeUsed[count];
    memset(modeUsed, 0, sizeof(bool) * count);

    // generate a mode count between 1 and the number of real modes
    int newCount = (rand() % count) + 1;
    drmModeModeInfoPtr newModes = (drmModeModeInfoPtr) malloc(sizeof(drmModeModeInfo) * newCount);
    memset(newModes, 0, sizeof(drmModeModeInfo) * newCount);

    // Shuffle modes from the old into the new
    for (int i=0; i<newCount; ++i)
    {
        // Choose one of the old modes that we haven't already used
        int n;
        do
        {
            n = rand() % count;
        } while (modeUsed[n]);

        newModes[i] = modes[n];
        newModes[i].type &= ~DRM_MODE_TYPE_PREFERRED;
        modeUsed[n] = true;
    }

    // Choose a preferred mode
    int prefModeIx = rand() % newCount;
    newModes[prefModeIx].type |= DRM_MODE_TYPE_PREFERRED;

    // Free the old mode list
    free(modes);

    // Return the shuffled modes to be passed back to HWC
    count = newCount;
    modes = newModes;
}

const char* DrmShimChecks::AspectStr(uint32_t aspect)
{
#if defined(DRM_PICTURE_ASPECT_RATIO)
    // Imin_legacy codepath
    switch(aspect)
    {
        case HDMI_PICTURE_ASPECT_4_3:
            return "4:3";
        case HDMI_PICTURE_ASPECT_16_9:
            return "16:9";
        default:
            break;
    }
#elif defined(DRM_MODE_FLAG_PARMASK)
    // Gmin codepath
    switch(aspect & DRM_MODE_FLAG_PARMASK)
    {
        case DRM_MODE_FLAG_PAR4_3:
            return "4:3";
        case DRM_MODE_FLAG_PAR16_9:
            return "16:9";
        default:
            break;
    }
#else
    HWCVAL_UNUSED(aspect);
#endif
    return "UNKNOWN_ASPECT";
}

void DrmShimChecks::LogModes(uint32_t connId, const char* str, drmModeConnectorPtr pConn)
{
    HWCLOGI("%s: connId %d encoder_id %d:", str, connId, pConn->encoder_id);
    for (int i=0; i<pConn->count_modes; ++i)
    {
        drmModeModeInfoPtr pMode = pConn->modes+i;
        HWCLOGI("  Mode %d: %s", i, pMode->name);
        HWCLOGI("  Clock %d vrefresh %d flags 0x%x aspect %s type %d %s",pMode->clock,
            pMode->vrefresh, pMode->flags, AspectStr(pMode->HWCVAL_DRM_ASPECT), pMode->type,
            ((pMode->type & DRM_MODE_TYPE_PREFERRED) ? "PREFERRED " : ""));
        HWCLOGI("  H Size %d sync start %d end %d total %d skew %d",
            pMode->hdisplay, pMode->hsync_start, pMode->hsync_end, pMode->htotal, pMode->hskew);
        HWCLOGI("  V Size %d sync start %d end %d total %d scan %d",
            pMode->vdisplay, pMode->vsync_start, pMode->vsync_end, pMode->vtotal, pMode->vscan);
    }

    if (pConn->count_modes != 1)
    {
        HWCLOGW("Number of modes=%d.", pConn->count_modes);
    }
}

static bool IsConnectorTypeHotPluggable(uint32_t connType)
{
    switch (connType)
    {
        case DRM_MODE_CONNECTOR_HDMIA:
        case DRM_MODE_CONNECTOR_HDMIB:
        case DRM_MODE_CONNECTOR_DisplayPort:
        {
            return true;
        }

        default:
        {
            return false;
        }
    }
}

void DrmShimChecks::CheckGetConnectorExit(int fd, uint32_t connId, drmModeConnectorPtr& pConn)
{
    HWCVAL_UNUSED(fd);
    HWCVAL_UNUSED(connId);
    HWCVAL_LOCK(_l, mMutex);

    HwcTestCrtc::ModeVec modes;

    LogModes(connId, "Real modes", pConn);

    // Optionally, pretend the panel is HDMI
    bool connectorPhysicallyHotPluggable = IsConnectorTypeHotPluggable(pConn->connector_type);

    if (mState->IsOptionEnabled(eOptSpoofNoPanel) && !connectorPhysicallyHotPluggable)
    {
        pConn->connector_type = DRM_MODE_CONNECTOR_HDMIA;
    }

    // Establish if the connector is software hot-pluggable (after allowing for display type spoof).
    bool hotPluggable = IsConnectorTypeHotPluggable (pConn->connector_type);

    if (hotPluggable)
    {
        // Note this will replace the existing item if is already there
        OverrideDefaultMode(pConn);
        mHotPluggableConnectors.add(pConn->connector_id);
    }
    else
    {
        mHotPluggableConnectors.remove(pConn->connector_id);
    }

    if ((pConn->count_modes > 1) && mState->IsOptionEnabled(eOptRandomizeModes))
    {
        RandomizeModes(pConn->count_modes, pConn->modes);
        LogModes(connId, "Shuffled modes", pConn);
    }

    uint32_t realRefresh = 0;

    // If we are spoofing DRRS and we are the panel, add the second mode for the minimum refresh
    if ((pConn->count_modes == 1) && (pConn->modes[0].vrefresh > 48) && mState->IsOptionEnabled(eOptSpoofDRRS))
    {
        drmModeModeInfoPtr pMode = pConn->modes;
        HwcTestCrtc::Mode mode;
        mode.width = pMode->hdisplay;
        mode.height = pMode->vdisplay;
        mode.refresh = pMode->vrefresh;
        mode.ratio = pMode->HWCVAL_DRM_ASPECT;
        mode.flags = 0;

        if (pMode->type & DRM_MODE_TYPE_PREFERRED)
        {
            mode.flags = HWCVAL_MODE_FLAG_PREFERRED;
        }

        modes.add(mode);

        char* pMem = (char*) malloc(2 * sizeof(drmModeModeInfo));
        memset(pMem, 0, 2 * sizeof(drmModeModeInfo));

        pConn->modes = (drmModeModeInfoPtr) pMem;
        pConn->modes[0] = *pMode;
        pConn->modes[1] = *pMode;
        pConn->modes[1].vrefresh = 48;
        pConn->count_modes = 2;
        free(pMode);

        realRefresh = mode.refresh;
        mode.refresh = 48;
        modes.add(mode);
    }
    else
    {
        for (int i=0; i<pConn->count_modes; ++i)
        {
            drmModeModeInfoPtr pMode = pConn->modes+i;
            HwcTestCrtc::Mode mode;
            mode.width = pMode->hdisplay;
            mode.height = pMode->vdisplay;
            mode.refresh = pMode->vrefresh;
            mode.ratio = pMode->HWCVAL_DRM_ASPECT;
            mode.flags = 0;

            if (pMode->type & DRM_MODE_TYPE_PREFERRED)
            {
                mode.flags = HWCVAL_MODE_FLAG_PREFERRED;
            }

            modes.add(mode);
        }
    }

    for (int i=0; i<pConn->count_encoders; ++i)
    {
        HWCLOGI("  Encoder %d", pConn->encoders[i]);
        mConnectorForEncoder.replaceValueFor(pConn->encoders[i], connId);
    }

    ssize_t ix = mConnectors.indexOfKey(connId);
    ALOG_ASSERT(mPropMgr);

    if (ix >= 0)
    {
        Connector& conn = mConnectors.editValueAt(ix);
        DrmShimCrtc* crtc = conn.mCrtc;
        conn.mModes = modes;
        conn.mAttributes = 0;
        conn.mRealRefresh = realRefresh;
        mPropMgr->CheckConnectorProperties(connId, conn.mAttributes);
        if (mState->IsOptionEnabled(eOptSpoofDRRS) && !connectorPhysicallyHotPluggable)
        {
            conn.mAttributes |= eDRRS;
        }

        conn.mRealDisplayType = connectorPhysicallyHotPluggable ? HwcTestState::eRemovable : HwcTestState::eFixed;

        if (crtc)
        {
            if (hotPluggable && (crtc->GetWidth() == 0))
            {
                // New CRTC
                // Use default connection state
                bool plug = mState->GetNewDisplayConnectionState();
                HWCLOGD_COND(eLogHotPlug, "Connector %d crtc %d using default connection state: %s",
                    connId, crtc->GetCrtcId(), plug ? "plug" : "unplug");
                crtc->SimulateHotPlug(plug);
            }

            crtc->SetAvailableModes(conn.mModes);

            if (!crtc->IsBehavingAsConnected())
            {
                HWCLOGD_COND(eLogHotPlug, "Connector %d CRTC %d hotplug spoof disconnected",
                    connId, crtc->GetCrtcId());
                pConn->connection = DRM_MODE_DISCONNECTED;
                pConn->count_modes = 0;
            }
        }
        else
        {
            HWCLOGD_COND(eLogHotPlug, "Connector %d known, but no CRTC", connId);

            if (hotPluggable && !mState->GetNewDisplayConnectionState())
            {
                HWCLOGD_COND(eLogHotPlug, "Connector %d initial spoof hotunplugged", connId);
                pConn->connection = DRM_MODE_DISCONNECTED;
                pConn->count_modes = 0;
            }
        }
    }
    else
    {
        Connector conn;
        conn.mCrtc = 0;
        conn.mModes = modes;
        conn.mAttributes = 0;
        conn.mRealRefresh = realRefresh;
        conn.mDisplayIx = eNoDisplayIx;
        mPropMgr->CheckConnectorProperties(connId, conn.mAttributes);

        if (mState->IsOptionEnabled(eOptSpoofDRRS) && !connectorPhysicallyHotPluggable)
        {
            conn.mAttributes |= eDRRS;
        }

        conn.mRealDisplayType = connectorPhysicallyHotPluggable ? HwcTestState::eRemovable : HwcTestState::eFixed;
        mConnectors.add(connId, conn);

        if (hotPluggable && !mState->GetNewDisplayConnectionState())
        {
            HWCLOGD_COND(eLogHotPlug, "New connector %d initial spoof hotunplugged", connId);
            pConn->connection = DRM_MODE_DISCONNECTED;
            pConn->count_modes = 0;
        }
        else
        {
            HWCLOGD_COND(eLogHotPlug, "New connector %d state %s", connId, (pConn->connection == DRM_MODE_CONNECTED) ? "connected" : "disconnected");
        }
    }

    // drmModeGetConnector can take ages which means hot plug is delayed
    // indicate that this is OK.
    mProtChecker.RestartSelfTeardown();
}

void DrmShimChecks::CheckGetEncoder(uint32_t encoder_id, drmModeEncoderPtr pEncoder)
{
    if (pEncoder)
    {
        HWCLOGI("DrmShimChecks::CheckGetEncoder encoder_id %d crtc_id %d possible_crtcs %d",
            encoder_id, pEncoder->crtc_id, pEncoder->possible_crtcs);
        mPossibleCrtcsForEncoder.replaceValueFor(encoder_id, pEncoder->possible_crtcs);
    }
}

void DrmShimChecks::MapDisplay(int32_t displayIx, uint32_t connId, uint32_t crtcId)
{
    if (displayIx >= 0)
    {
        ssize_t ix = mConnectors.indexOfKey(connId);

        if (ix >= 0)
        {
            Connector& conn = mConnectors.editValueAt(ix);

            HWCLOGI("MapDisplay: Connector %d -> displayIx %d (%d modes) crtc %d@%p",
                connId, displayIx, conn.mModes.size(), (conn.mCrtc ? conn.mCrtc->GetCrtcId() : 0), conn.mCrtc);
            conn.mDisplayIx = displayIx;

            if (conn.mCrtc)
            {
                if (conn.mCrtc->GetCrtcId() != crtcId)
                {
                    HWCLOGW("Inconsistent connector-CRTC mapping. HWC says connector %d is crtc %d, we think crtc %d",
                        connId, crtcId, conn.mCrtc->GetCrtcId());
                }
            }
        }
        else
        {
            HWCLOGW("MapDisplay: Connector %d UNKNOWN displayIx %d", connId, displayIx);
        }
    }
}

void DrmShimChecks::CheckSetCrtcEnter ( int fd, uint32_t crtcId,
                                        uint32_t bufferId,
                                        uint32_t x, uint32_t y,
                                        uint32_t *connectors, int count,
                                        drmModeModeInfoPtr mode)
{
    HWCLOGI("DrmShimChecks::CheckSetCrtcEnter @ %p: Crtc %d:", this, crtcId);
    HWCVAL_UNUSED(fd);
    HWCVAL_UNUSED(x);
    HWCVAL_UNUSED(y);

    if (!mode)
    {
        HWCLOGA("  No mode");
        return;
    }

    HWCLOGA("  Crtc %d Mode %s clock %d vrefresh %d flags %x aspect %s type %d",
        crtcId, mode->name, mode->clock, mode->vrefresh, mode->flags, AspectStr(mode->HWCVAL_DRM_ASPECT), mode->type);
    HWCLOGI("  H Size %d sync start %d end %d total %d skew %d",
        mode->hdisplay, mode->hsync_start, mode->hsync_end, mode->htotal, mode->hskew);
    HWCLOGI("  V Size %d sync start %d end %d total %d scan %d",
        mode->vdisplay, mode->vsync_start, mode->vsync_end, mode->vtotal, mode->vscan);

    HWCVAL_LOCK(_l,mMutex);
    mWorkQueue.Process();

    HwcTestState::DisplayType displayType = HwcTestState::eFixed;
    int pipe = 0;

    // Determine display type
    for (int i=0; i<count; ++i)
    {
        if (mHotPluggableConnectors.indexOf(connectors[i]) >= 0)
        {
            displayType = HwcTestState::eRemovable;
            pipe = i;
        }
    }

    // Do we already know about the CRTC?
    ssize_t ix = mCrtcs.indexOfKey(crtcId);
    DrmShimCrtc* crtc;

    if ((mCrtcByPipe[pipe] == 0) && (ix < 0))
    {
        // No
        // So let's create it
        crtc = new DrmShimCrtc(crtcId, mode->hdisplay, mode->vdisplay, mode->clock, mode->vrefresh);
        crtc->SetChecks(this);
        crtc->SetPipeIndex(pipe);
        HwcTestCrtc::SeqVector* seq = mOrders.valueFor(0);
        crtc->SetZOrder(seq);

        mCrtcs.add(crtcId, crtc);
        mCrtcByPipe[pipe] = crtc;
        HWCLOGD("Pipe %d has new CRTC %d Dimensions %dx%d clock %d refresh %d",
            pipe, crtcId, mode->hdisplay, mode->vdisplay, mode->clock, mode->vrefresh);
    }
    else if (ix < 0)
    {
        crtc = mCrtcByPipe[pipe];
        HWCLOGD("Pipe %d CRTC %d maps to existing CRTC %d", pipe, crtcId, crtc->GetCrtcId());
        crtc->SetCrtcId(crtcId);
        mCrtcs.add(crtcId, crtc);
    }
    else
    {
        crtc = mCrtcs.valueAt(ix);
        HWCLOGD("Reset mode for CRTC %d to %dx%d@%d", crtcId, mode->hdisplay, mode->vdisplay, mode->vrefresh);
    }

    crtc->SetDisplayType(displayType);

    HwcTestCrtc::Mode actualMode;
    actualMode.width = mode->hdisplay;
    actualMode.height = mode->vdisplay;
    actualMode.refresh = mode->vrefresh;
    actualMode.flags = mode->flags;
    actualMode.ratio = mode->HWCVAL_DRM_ASPECT;
    crtc->SetActualMode(actualMode);

    ix = mPlanes.indexOfKey(crtcId);
    DrmShimPlane* mainPlane = 0;

    if (ix < 0)
    {
        if (!mUniversalPlanes)
        {
            HWCLOGD("Universal planes DISABLED: Creating main plane %d for crtc %d", crtcId, crtcId);
            // also create the main plane
            mainPlane = new DrmShimPlane(crtcId, crtc);
            mainPlane->SetPlaneIndex(0);
            mPlanes.add(crtcId, mainPlane);
            crtc->AddPlane(mainPlane);
        }
    }
    else
    {
        mainPlane = mPlanes.valueAt(ix);
    }

    // Remember the connector->CRTC associations
    for (int i=0; i<count; ++i)
    {
        ssize_t ix = mConnectors.indexOfKey(connectors[i]);
        uint32_t dix;

        if (ix >= 0)
        {
            Connector& conn = mConnectors.editValueAt(ix);

            // If we are spoofing DRRS
            // make sure we don't send weird refresh rates to DRM that can't deal with it.
            if (conn.mRealRefresh > 0)
            {
                mode->vrefresh = conn.mRealRefresh;
            }

            conn.mCrtc = crtc;
            crtc->SetDisplayIx(conn.mDisplayIx);
            crtc->SetRealDisplayType(conn.mRealDisplayType);

            if (conn.mDisplayIx != eNoDisplayIx)
            {
                mCrtcByDisplayIx[conn.mDisplayIx] = crtc;
                mPersistentCrtcByDisplayIx[conn.mDisplayIx] = crtc;
            }

            HWCLOGI("  Connector %d -> CRTC %d D%d (%d modes)",connectors[i], crtc->GetCrtcId(), crtc->GetDisplayIx(), conn.mModes.size());
            crtc->SetAvailableModes(conn.mModes);
            dix = conn.mDisplayIx;

            if ((dix == 0) && (crtc->GetWidth() != 0))
            {
                // D0 is being resized.
                // HWC will use a proxy, so the input dimensions will not change.
                HWCLOGD("D%d Crtc %d Setting OutDimensions %dx%d",
                    crtc->GetDisplayIx(), crtc->GetCrtcId(), mode->hdisplay, mode->vdisplay);
                crtc->SetOutDimensions(mode->hdisplay, mode->vdisplay);
            }
            else
            {
                HWCLOGD("D%d Crtc %d Setting Dimensions %dx%d clock %d refresh %d",
                    crtc->GetDisplayIx(), crtc->GetCrtcId(), mode->hdisplay, mode->vdisplay, mode->clock, mode->vrefresh);
                crtc->SetDimensions(mode->hdisplay, mode->vdisplay, mode->clock, mode->vrefresh);
            }

            // Is there a logical display mapping set up? If so use it
            const char* ldmStr = mState->GetHwcOptionStr("dmconfig");

            if (ldmStr)
            {
                HWCLOGD("Logical display config will override: %s", ldmStr);
                ParseDmConfig(ldmStr);
            }
            else
            {
                HWCLOGD_COND(eLogMosaic, "No logical display config (dmconfig)");
            }
        }
        else
        {
            Connector conn;
            conn.mCrtc = crtc;
            conn.mDisplayIx = crtc->GetDisplayIx();
            dix = conn.mDisplayIx;
            mConnectors.add(connectors[i], conn);
            HWCLOGI("  Connector %d UNKNOWN -> CRTC %d D%d",connectors[i], crtc->GetCrtcId(), crtc->GetDisplayIx());
            ALOG_ASSERT(0);
        }

        crtc->SetConnector(connectors[i]);

        // Make sure no other CRTCs point to the same connector
        for (uint32_t j=0; j<mCrtcs.size(); ++j)
        {
            DrmShimCrtc* otherCrtc = mCrtcs.valueAt(j);

            if (otherCrtc != crtc)
            {
                if (otherCrtc->GetDisplayIx() == dix)
                {
                    otherCrtc->SetDisplayIx(-1);
                }
            }

            HWCLOGV_COND(eLogDrm, "Crtc %d -> D%d", otherCrtc->GetCrtcId(), otherCrtc->GetDisplayIx());
        }

        for (uint32_t d=0; d<HWCVAL_MAX_CRTCS; ++d)
        {
            if (mCrtcByDisplayIx[d])
            {
                HWCLOGV_COND(eLogDrm, "D%d -> Crtc %d", d, mCrtcByDisplayIx[d]->GetCrtcId());
            }
        }
    }

    // We were also supplied a buffer id, so behave as in SetPlane
    if ((bufferId != 0) && (mainPlane != 0))
    {
        ssize_t bufIx = mBuffersByFbId.indexOfKey(bufferId);
        if (bufIx < 0)
        {
            // This buffer not known to us
            // probably blanking
            mainPlane->ClearBuf();
        }
        else
        {
            android::sp<DrmShimBuffer> buf = UpdateBufferPlane(bufferId, crtc, mainPlane);
        }
    }

    // Clear ESD recovery
    crtc->EsdStateTransition(HwcTestCrtc::eEsdDpmsOff, HwcTestCrtc::eEsdModeSet);

    // Complete invalidation of protected sessions that was started by hot plug
    mProtChecker.InvalidateOnModeChange();
}

void DrmShimChecks::CheckSetCrtcExit  ( int fd, uint32_t crtcId,
                                        uint32_t ret )
{
    HWCLOGD("DrmShimChecks::CheckSetCrtcExit @ %p: Crtc %d:", this, crtcId);
    HWCVAL_UNUSED(fd);

    HWCVAL_LOCK(_l,mMutex);
    mWorkQueue.Process();

    HwcTestCrtc* crtc = GetCrtc(crtcId);

    if (ret == 0)
    {
        crtc->SetModeSet(true);
    }
    else
    {
        HWCERROR(eCheckDrmCallSuccess, "drmModeSetCrtcExit failed to CRTC %d", crtcId);
    }
}


void DrmShimChecks::CheckGetCrtcExit(uint32_t crtcId, drmModeCrtcPtr pCrtc)
{
    HWCLOGI("GetCrtc: Crtc %d:",crtcId);
    HWCLOGI("  Mode %s", pCrtc->mode.name);
    HWCLOGI("  Clock %d vrefresh %d flags %d type %d",pCrtc->mode.clock,
        pCrtc->mode.vrefresh, pCrtc->mode.flags, pCrtc->mode.type);
    HWCLOGI("  H Size %d sync start %d end %d total %d skew %d",
        pCrtc->mode.hdisplay, pCrtc->mode.hsync_start, pCrtc->mode.hsync_end, pCrtc->mode.htotal, pCrtc->mode.hskew);
    HWCLOGI("  V Size %d sync start %d end %d total %d scan %d",
        pCrtc->mode.vdisplay, pCrtc->mode.vsync_start, pCrtc->mode.vsync_end, pCrtc->mode.vtotal, pCrtc->mode.vscan);

    // Create CRTC record on drmModeSetCrtc, not here
    // since we have no idea what display index is at this point.
}

/// Check for drmModeGetPlaneResources
void DrmShimChecks::CheckGetPlaneResourcesExit(drmModePlaneResPtr pRes)
{
    HWCVAL_LOCK(_l,mMutex);
    // Record the existence of the "new" planes
    for (uint32_t i=0; i < pRes->count_planes; ++i)
    {
        if (mPlanes.indexOfKey(pRes->planes[i]) < 0)
        {
            DrmShimPlane* plane = new DrmShimPlane(pRes->planes[i]);

            HWCLOGI("GetPlaneResources: new plane %d", pRes->planes[i]);
            mPlanes.add(pRes->planes[i], plane);
        }
    }
}

DrmShimCrtc* DrmShimChecks::CreatePipe(uint32_t pipe, uint32_t crtcId)
{
    DrmShimCrtc* crtc = mCrtcByPipe[pipe];

    if (crtc == 0)
    {
        // Create the crtc, drmModeGetCrtc has not been called yet.
        // So, we don't yet know the CRTC id
        HWCLOGD("Creating new CRTC %d for pipe %d with unknown CRTC id", crtcId, pipe);
        crtc = new DrmShimCrtc(crtcId, 0, 0, 0, 0);
        crtc->SetChecks(this);
        crtc->SetPipeIndex(pipe);
        HwcTestCrtc::SeqVector* seq = mOrders.valueFor(0);
        crtc->SetZOrder(seq);
        mCrtcByPipe[pipe] = crtc;
    }
    else if ((crtcId > 0) && (crtcId != crtc->GetCrtcId()))
    {
        HWCLOGW("Pipe %d existing CRTC has id %d, should be %d",
            pipe, crtc->GetCrtcId(), crtcId);
        ALOG_ASSERT(crtcId == crtc->GetCrtcId());
    }

    return crtc;
}


/// Check for drmModeGetPlane
void DrmShimChecks::CheckGetPlaneExit(uint32_t plane_id, drmModePlanePtr pPlane)
{
    HWCVAL_LOCK(_l,mMutex);

    // Record the association between the plane and Crtc
    // Note: pPlane->crtc_id is actually null, so we have to use the last CRTC Id set in GetCrtc
    ssize_t planeIx = mPlanes.indexOfKey(plane_id);

    uint32_t pipe = 0;
    while (( ( (1 << pipe) & (pPlane->possible_crtcs) ) == 0) && (pipe < HWCVAL_MAX_CRTCS))
    {
        ++pipe;
    }
    HWCLOGD("CheckGetPlaneExit: plane %d possible_crtcs 0x%x crtc_id %d planeIx %d pipe %d",
      plane_id, pPlane->possible_crtcs, pPlane->crtc_id, planeIx, pipe);

    if ( ((uint32_t)(1 << pipe)) != pPlane->possible_crtcs)
    {
        HWCERROR(eCheckDrmShimFail, "Plane %d mapped to multiple/unknown CRTCs. possible_crtcs=0x%x",
          plane_id, pPlane->possible_crtcs);
        return;
    }

    if (planeIx >= 0)
    {
        DrmShimPlane* plane = mPlanes.valueAt(planeIx);
        if (pipe < HWCVAL_MAX_CRTCS)
        {
            DrmShimCrtc* crtc = CreatePipe(pipe);

            plane->SetCrtc(crtc);

#ifdef DRM_PLANE_TYPE_CURSOR
            int32_t plane_type = mPropMgr->GetPlaneType(plane_id);
            if (plane_type == DRM_PLANE_TYPE_CURSOR)
            {
                HWCLOGD("CheckGetPlaneExit: NOT adding cursor plane %d to crtc %d", plane_id, (crtc ? crtc->GetCrtcId() : 0));
            }
            else
            {
#endif
                HWCLOGD("CheckGetPlaneExit: adding plane %p to crtc %d", plane, (crtc ? crtc->GetCrtcId() : 0));
                crtc->AddPlane(plane);
#ifdef DRM_PLANE_TYPE_CURSOR
            }
#endif

            HWCLOGI("CheckGetPlaneExit: plane %d possible_crtcs 0x%x associated with crtc %d", plane_id, pPlane->possible_crtcs,
              (crtc ? crtc->GetCrtcId() : 0));
        }
        else
        {
            HWCLOGW("CheckGetPlaneExit: Crtc for pipe %d not valid", pipe);
        }
    }
    else
    {
        HWCLOGI("CheckGetPlaneExit: plane %d not previously found by GetPlaneResources", plane_id);
    }

    // For nuclear spoofing, ensure the main plane has all the right formats.
    if (mState->IsOptionEnabled(eOptSpoofNuclear))
    {
        HWCLOGD("Plane %d has %d formats. [0]=0x%x", plane_id, pPlane->count_formats, pPlane->formats ? pPlane->formats[0] : 0);
        if (pPlane->count_formats <= 2)
        {
            // This will be the main plane.
            HWCLOGD("CheckGetPlaneExit: Spoofing formats for plane %d", plane_id);
            uint32_t* oldFormats = pPlane->formats;
            uint32_t n = pPlane->count_formats;
            pPlane->count_formats += 4;
            pPlane->formats = (uint32_t*) malloc(sizeof(uint32_t) * pPlane->count_formats);
            pPlane->formats[n++] = DRM_FORMAT_ARGB8888;
            pPlane->formats[n++] = DRM_FORMAT_ABGR8888;
            pPlane->formats[n++] = DRM_FORMAT_XBGR8888;
            pPlane->formats[n++] = DRM_FORMAT_RGB565;
            ALOG_ASSERT(n == pPlane->count_formats);
            free(oldFormats);
        }
    }
}

/// Check for drmModeAddFB and drmModeAddFB2
void DrmShimChecks::CheckAddFB(int fd, uint32_t width, uint32_t height,
                                    uint32_t pixel_format,
                                    uint32_t depth,
                                    uint32_t bpp,
                                    uint32_t bo_handles[4],
                                    uint32_t pitches[4],
                                    uint32_t offsets[4],
                                    uint32_t buf_id,
                                    uint32_t flags,
                                    __u64 modifier[4],
                                    int ret)
{
    HWCVAL_UNUSED(width);
    HWCVAL_UNUSED(height);

    // AddFB can take up to four handles, for cases where the channels are in separate buffers.
    // At time of writing this is not required for any buffer HWC would give an FB to.
    // This will change in future when NV12 buffers are supported by hardware.
    uint32_t boHandle = bo_handles[0];

    if ((ret == 0) && (buf_id > 0))
    {
        HWCLOGV_COND(eLogDrm, "drmModeAddFB: buf_id %d pixel_format 0x%x depth %d bpp %d "
            "boHandles/pitches/offsets/modifier (0x%x/%d/%d/%llu,0x%x/%d/%d/%llu,0x%x/%d/%d/%llu,0x%x/%d/%d/%llu) flags %d",
            buf_id, pixel_format, depth, bpp,
            bo_handles[0], pitches[0], offsets[0], modifier[0],
            bo_handles[1], pitches[1], offsets[1], modifier[1],
            bo_handles[2], pitches[2], offsets[2], modifier[2],
            bo_handles[3], pitches[3], offsets[3], modifier[3],
            flags);

        if (flags & DRM_MODE_FB_AUX_PLANE)
        {
            // Aux buffer detected - save the pitch, offset and modifier
            HWCLOGV_COND(eLogDrm, "drmModeAddFB: Aux buffer detected for buf_id %d - pitch is %d - offset is %d - modifier is %llu",
                buf_id, pitches[1], offsets[1], modifier[1]);
            mWorkQueue.Push(new Hwcval::Work::AddFbItem(fd, boHandle, buf_id, width, height, pixel_format, pitches[1], offsets[1], modifier[1]));
        }
        else
        {
            mWorkQueue.Push(new Hwcval::Work::AddFbItem(fd, boHandle, buf_id, width, height, pixel_format));
        }
    }
    else
    {
        HWCLOGW("drmModeAddFB handle 0x%x failed to allocate FB ID %d status %d", boHandle, buf_id, ret);
        HWCLOGD("buf_id %d pixel_format 0x%x depth %d bpp %d boHandles/pitches/offsets (0x%x/%d/%d,0x%x/%d/%d,0x%x/%d/%d,0x%x/%d/%d) flags %d",
            buf_id, pixel_format, depth, bpp,
            bo_handles[0], pitches[0], offsets[0],
            bo_handles[1], pitches[1], offsets[1],
            bo_handles[2], pitches[2], offsets[2],
            bo_handles[3], pitches[3], offsets[3],
            flags);
    }
}

void DrmShimChecks::CheckRmFB(int fd, uint32_t bufferId)
{
    mWorkQueue.Push(new Hwcval::Work::RmFbItem(fd, bufferId));
}

/// Work queue processing for drmModeAddFB and drmModeAddFB2
///
/// These associate a framebuffer id (FB ID) with a buffer object (bo).
void DrmShimChecks::DoWork(const Hwcval::Work::AddFbItem& item)
{
    char str[HWCVAL_DEFAULT_STRLEN];
    uint32_t pixelFormat = item.mPixelFormat;
    uint32_t auxPitch = item.mAuxPitch;
    uint32_t auxOffset = item.mAuxOffset;
    __u64 modifier = item.mModifier;

    if (item.mHasAuxBuffer)
    {
        HWCLOGD("DoWork AddFbItem FB %d fd %d boHandle 0x%x (Aux buffer detected - pitch: %d offset: %d modifier: %llu)",
            item.mFbId, item.mFd, item.mBoHandle, auxPitch, auxOffset, modifier);
    }
    else
    {
        HWCLOGD("DoWork AddFbItem FB %d fd %d boHandle 0x%x", item.mFbId, item.mFd, item.mBoHandle);
    }

    BoKey k = {item.mFd, item.mBoHandle};
    ssize_t ix = mBosByBoHandle.indexOfKey(k);

    if (ix >= 0)
    {
        // We found the bo record for this bo handle, add the FB to the DrmShimBuffer
        // and make sure it is indexed.
        android::sp<HwcTestBufferObject> bo = mBosByBoHandle.valueAt(ix);
        android::sp<DrmShimBuffer> buf = bo->mBuf.promote();
        HWCLOGD_COND(eLogBuffer,"AddFb found bo %s, buf@%p", bo->IdStr(str), buf.get());

        if (buf.get())
        {
            DrmShimBuffer::FbIdData data;
            data.pixelFormat = pixelFormat;
            data.hasAuxBuffer = item.mHasAuxBuffer;
            data.auxPitch = auxPitch;
            data.auxOffset = auxOffset;
            data.modifier = modifier;

            buf->GetFbIds().add(item.mFbId, data);
            mBuffersByFbId.replaceValueFor(item.mFbId, buf);
            // TODO: what if this FB ID previously belonged to a different buffer?

            HWCLOGD_COND(eLogBuffer, "drmModeAddFB[2]: Add FB %d to %s pixelFormat 0x%x", item.mFbId, buf->IdStr(str), pixelFormat);
        }
        else
        {
            // Sometimes the addFB comes before the create. Why??
            //
            // Create a dummy DrmShimBuffer, this will get more information later
            // when RecordBufferState is called.
            buf = new DrmShimBuffer(0);

            // Assume this is a blanking or empty buffer until it is associated with a handle.
            // We decide which based on the size of the buffer - this is based on our knowledge of how HWC works.
            if (BelievedEmpty(item.mWidth, item.mHeight))
            {
                buf->SetBlack(true);
            }
            else
            {
                buf->SetBlanking(true);
            }

            // Associate the bo with the DrmShimBuffer.
            bo->mBuf = buf;
            buf->AddBo(bo);

            // Add the FB ID to the new buffer
            DrmShimBuffer::FbIdData data;
            data.pixelFormat = pixelFormat;
            data.hasAuxBuffer = item.mHasAuxBuffer;
            data.auxPitch = auxPitch;
            data.auxOffset = auxOffset;
            data.modifier = modifier;

            buf->GetFbIds().add(item.mFbId, data);
            mBuffersByFbId.replaceValueFor(item.mFbId, buf);
            // TODO: what if this FB ID previously belonged to a different buffer?

            char str[HWCVAL_DEFAULT_STRLEN];
            HWCLOGD_COND(eLogBuffer, "drmModeAddFB[2]: Add FB %d to new %s pixelFormat 0x%x", item.mFbId, buf->IdStr(str), pixelFormat);
        }
    }
    else
    {
        // We don't know about this bo handle.
        // This can happen sometimes - the AddFB happens before the bo is apparently created. Or it could be that we
        // previously dispensed with a bo record because it didn't seem to be used any more, and now we need it again.
        // Either way, we create a new buffer object and associate it with the FB ID.
        android::sp<HwcTestBufferObject> bo = new HwcTestBufferObject(item.mFd, item.mBoHandle);
        DrmShimBuffer::FbIdData data;
        data.pixelFormat = pixelFormat;
        data.hasAuxBuffer = item.mHasAuxBuffer;
        data.auxPitch = auxPitch;
        data.auxOffset = auxOffset;
        data.modifier = modifier;
        android::sp<DrmShimBuffer> buf = new DrmShimBuffer(0);
        buf->GetFbIds().add(item.mFbId, data);
        mBuffersByFbId.replaceValueFor(item.mFbId, buf);
        mBosByBoHandle.add(k, bo);
        buf->AddBo(bo);
        // TODO: what if this FB ID previously belonged to a different buffer?

        char str[HWCVAL_DEFAULT_STRLEN];
        HWCLOGD_COND(eLogBuffer, "drmModeAddFB[2]: NEW FB %d %s pixelFormat 0x%x", item.mFbId, bo->IdStr(str), pixelFormat);
    }
}

/// Work queue processing for drmModeRmFB.
/// This remove a framebuffer ID (FB ID) from a buffer object.
void DrmShimChecks::DoWork(const Hwcval::Work::RmFbItem& item)
{
    char strbuf[HWCVAL_DEFAULT_STRLEN];
    ssize_t ix = mBuffersByFbId.indexOfKey(item.mFbId);

    if (ix >= 0)
    {
        // We found the bo from the FB ID index, so remove the FB ID from the BO and from the index.
        android::sp<DrmShimBuffer> buf = mBuffersByFbId.valueAt(ix);
        buf->GetFbIds().removeItem(item.mFbId);
        mBuffersByFbId.removeItemsAt(ix);

        HWCLOGD_COND(eLogBuffer, "drmModeRmFB: Removed association of FB %d with %s", item.mFbId, buf->IdStr(strbuf));
    }
    else
    {
        // RmFB may happen after the buffer object has already been closed, so this is not an error.
        HWCLOGW_COND(eLogBuffer, "drmModeRmFB: Unknown FB ID %d", item.mFbId);
    }
}

void DrmShimChecks::checkPageFlipEnter(int fd, uint32_t crtc_id,
                                       uint32_t fb_id, uint32_t flags,
                                       void *&user_data)
{
    if (mState->IsCheckEnabled(eLogDrm))
    {
         HWCLOGD("Enter DrmShimChecks::checkPageFlipEnter fd %x crtc_id %d FB %d flags %x user_data %p",fd, crtc_id, fb_id, flags, user_data);
    }

    if (mState->IsBufferMonitorEnabled())
    {
        android::sp<DrmShimBuffer> buf;
        {
            // Note, this lock must be released BEFORE mCompVal->Compare() is called.
            HWCVAL_LOCK(_l,mMutex);
            mWorkQueue.Process();

            DrmShimPlane* mainPlane = mPlanes.valueFor(crtc_id);
            HWCCHECK(eCheckInvalidCrtc);
            if (mainPlane == 0)
            {
                HWCERROR(eCheckInvalidCrtc, "Unknown CRTC %d",crtc_id);
                return;
            }

            uint32_t fbForCrtc = mainPlane->GetCurrentDsId();

            // Get pointer to internal CRTC object
            DrmShimCrtc* crtc = static_cast<DrmShimCrtc*>(mainPlane->GetCrtc());
            HWCCHECK(eCheckInvalidCrtc);
            if (crtc == 0)
            {
                HWCERROR(eCheckInvalidCrtc, "Could not find a crtc entry for id %d", crtc_id);
                return;
            }

            // Not a dropped frame
            crtc->IncDrawCount();

            // Not disabled (anymore)
            crtc->SetMainPlaneDisabled(false);

            // Record frame number to check for flicker
            crtc->SetDrmFrame();
            mainPlane->DrmCallStart();

            if (mState->IsOptionEnabled(eOptPageFlipInterception))
            {
                if (user_data)
                {
                    HWCLOGD_COND(eLogEventHandler, "Crtc %d saving user data %p", crtc->GetCrtcId(), user_data);
                    crtc->SavePageFlipUserData(uint64_t(user_data));
                    user_data = (void*) (uintptr_t) crtc->GetCrtcId();
                    HWCLOGD_COND(eLogEventHandler, "Crtc %d Page flip user data shimmed with crtc %p", crtc->GetCrtcId(), user_data);
                }
            }

            if (fbForCrtc == fb_id)
            {
            }
            else
            {
                if (fb_id != 0)
                {
                    ssize_t bufIx = mBuffersByFbId.indexOfKey(fb_id);
                    if (bufIx < 0)
                    {
                        // This buffer not known to us
                        // probably blanking
                        mainPlane->ClearBuf();
                        return;
                    }

                    buf = UpdateBufferPlane(fb_id, crtc, mainPlane);

                    if (buf.get() == 0)
                    {
                        // Blanking buffer
                        return;
                    }

                    // Set expected dimensions
                    mainPlane->SetDisplayFrame(0, 0, buf->GetWidth(), buf->GetHeight());
                    mainPlane->SetSourceCrop(0, 0, buf->GetWidth(), buf->GetHeight());

                    if (buf->GetHandle() != 0)
                    {
                        // Allocated size of buffer must be at least full screen
                        HWCCHECK(eCheckMainPlaneFullScreen);
                        if ((buf->GetAllocWidth() < crtc->GetWidth()) ||
                             buf->GetAllocHeight() < crtc->GetHeight())
                        {
                            HWCERROR(eCheckMainPlaneFullScreen, "Size is %dx%d", buf->GetAllocWidth(), buf->GetAllocHeight());
                        }
                    }
                }
                else
                {
                    mainPlane->ClearBuf();
                }
            }
        }

        // Check the composition that created this buffer, if there was one
        mCompVal->Compare(buf);
    }
}

HwcTestBufferObject* DrmShimChecks::CreateBufferObject(int fd, uint32_t boHandle)
{
    return new HwcTestBufferObject(fd, boHandle);
}

android::sp<HwcTestBufferObject> DrmShimChecks::GetBufferObject(uint32_t boHandle)
{
    char strbuf[HWCVAL_DEFAULT_STRLEN];
    BoKey k = {mShimDrmFd, boHandle};
    android::sp<HwcTestBufferObject> bo = mBosByBoHandle.valueFor(k);

    if (bo.get() == 0)
    {
        bo = CreateBufferObject(mShimDrmFd, boHandle);
        HWCLOGV_COND(eLogBuffer, "GetBufferObject: fd %d boHandle 0x%x created %s", mShimDrmFd, boHandle, bo->IdStr(strbuf));
        mBosByBoHandle.add(k, bo);
    }
    else
    {
        HWCLOGV_COND(eLogBuffer, "GetBufferObject: fd %d boHandle 0x%x found %s", mShimDrmFd, boHandle, bo->IdStr(strbuf));
    }

    return bo;
}

void DrmShimChecks::checkPageFlipExit(int fd, uint32_t crtc_id,
                                       uint32_t fb_id, uint32_t flags,
                                       void *user_data,
                                       int ret)
{
    HWCLOGV_COND(eLogDrm, "Enter DrmShimChecks::checkPageFlipExit fd %x crtc_id %d FB %d flags %x user_data %p",fd, crtc_id, fb_id, flags, user_data);

    HWCCHECK(eCheckDrmCallSuccess);
    if (ret)
    {
        HWCERROR(eCheckDrmCallSuccess, "Page flip failed to crtc %d (status %d)", crtc_id, ret);
    }

    if (mState->IsBufferMonitorEnabled())
    {
        HWCVAL_LOCK(_l,mMutex);
        mWorkQueue.Process();

        DrmShimPlane* mainPlane = mPlanes.valueFor(crtc_id);
        if (mainPlane == 0)
        {
            return;
        }

        int32_t callDuration = mainPlane->GetDrmCallDuration();

        if (callDuration > HWCVAL_DRM_CALL_DURATION_WARNING_LEVEL_NS)
        {
            HWCLOGW("PageFlip to crtc %d took %fms", crtc_id, ((double) callDuration) / 1000000.0);
        }

        DrmShimCrtc* crtc = static_cast<DrmShimCrtc*>(mainPlane->GetCrtc());

        // Record frame number to check for flicker
        crtc->SetDrmFrame();
     }

}

void DrmShimChecks::checkSetPlaneEnter(int fd, uint32_t plane_id,
                                        uint32_t crtc_id,
                                        uint32_t fb_id, uint32_t flags,
                                        uint32_t crtc_x, uint32_t crtc_y,
                                        uint32_t crtc_w, uint32_t crtc_h,
                                        uint32_t src_x, uint32_t src_y,
                                        uint32_t src_w, uint32_t src_h,
                                        void*& user_data)
{
    char strbuf[HWCVAL_DEFAULT_STRLEN];

    if (mState->IsCheckEnabled(eLogDrm))
    {
        HWCLOGD("Enter DrmShimChecks::checkSetPlaneEnter");
        HWCLOGD("  -- fd %x plane id %d crtc_id %d FB %u flags %d ud %p", fd, plane_id, crtc_id, fb_id, flags, user_data);
        HWCLOGD("  -- src x,y,w,h (%4.2f, %4.2f, %4.2f, %4.2f) crtc (%d, %d, %d, %d)",
          (double)src_x/65536.0, (double)src_y/65536.0, (double)src_w/65536.0, (double)src_h/65536.0, crtc_x, crtc_y, crtc_w, crtc_h);
    }

    if (mState->IsBufferMonitorEnabled())
    {
        android::sp<DrmShimBuffer> buf;

        {
            // Note, this lock must be released BEFORE mCompVal->Compare() is called.
            HWCVAL_LOCK(_l,mMutex);
            mWorkQueue.Process();

            DrmShimPlane* plane = mPlanes.valueFor(plane_id);

            HWCCHECK(eCheckPlaneIdInvalidForCrtc);
            if (plane == 0)
            {
                HWCERROR(eCheckPlaneIdInvalidForCrtc, "Unknown plane %d", plane_id);
                return;
            }

            // Get pointer to internal CRTC object
            DrmShimCrtc* crtc = static_cast<DrmShimCrtc*>(plane->GetCrtc());
            // HWCCHECK already done
            if (crtc == 0)
            {
                HWCERROR(eCheckPlaneIdInvalidForCrtc, "No entry for crtc %d on plane %d", crtc_id, plane_id);
                return;
            }

            // HWCCHECK already done
            if (crtc->GetCrtcId() != crtc_id)
            {
                HWCERROR(eCheckPlaneIdInvalidForCrtc, "Plane %d sent to wrong CRTC %d", plane_id, crtc_id);
                return;
            }

             // Not a dropped frame
            crtc->IncDrawCount();

            // Record frame number to check for flicker
            crtc->SetDrmFrame();
            plane->DrmCallStart();

            if (mState->IsOptionEnabled(eOptPageFlipInterception))
            {
                if (user_data)
                {
                    // Setup page flip event
                    HWCLOGD_COND(eLogEventHandler, "Crtc %d saving user data %p", crtc->GetCrtcId(), user_data);
                    crtc->SavePageFlipUserData(uint64_t(user_data));
                    user_data = (void*) (uintptr_t) crtc_id;
                    HWCLOGD_COND(eLogEventHandler, "Crtc %d Page flip user data shimmed with crtc %p", crtc->GetCrtcId(), user_data);
                }
            }

            if (plane->GetCurrentDsId() == fb_id)
            {
                // Set expected dimensions
                plane->SetDisplayFrame(crtc_x, crtc_y, crtc_w, crtc_h);
                plane->SetSourceCrop(
                    float(src_x)/65536.0, float(src_y)/65536.0, float(src_w)/65536.0, float(src_h)/65536.0);

                // check for disabling main plane
                // TODO: check proper usage of this flag
                if (flags & DRM_MODE_PAGE_FLIP_EVENT)
                {
                    HWCLOGD("Detected callback to force main plane disabled on FB %d plane %d",fb_id, plane_id);
                    plane->GetCrtc()->SetMainPlaneDisabled(true);
                }
            }
            else
            {
                // fb_id=0 means we are turning off the plane.
                if (fb_id != 0)
                {
                    // check for disabling main plane
                    // TODO: check proper usage of this flag
                    if (flags & DRM_MODE_PAGE_FLIP_EVENT)
                    {
                        HWCLOGD("Detected callback to force main plane disabled on FB %d plane %d",fb_id, plane_id);
                        plane->GetCrtc()->SetMainPlaneDisabled(true);
                    }

                    buf = UpdateBufferPlane(fb_id, crtc, plane);
                    double w = ((double) src_w) / 65536.0;
                    double h = ((double) src_h) / 65536.0;


                    // Null DrmShimBuffer implies blanking.
                    if (buf.get())
                    {
                        // TODO: check if fence indicates that buffer is ready for display

                        // Don't perform checks against alloc width & height for blanking buffers
                        // as gralloc often only gives zeroes for these.
                        if (!buf->IsBlanking() && !buf->IsBlack())
                        {
                            HWCCHECK(eCheckBufferTooSmall);
                            if ( (w > buf->GetAllocWidth()) || (h > buf->GetAllocHeight()) )
                            {
                                HWCERROR(eCheckBufferTooSmall, "Plane %d %s %dx%d (alloc %dx%d) Crop %fx%f Display %dx%d",
                                    plane_id, buf->IdStr(strbuf), buf->GetWidth(), buf->GetHeight(), buf->GetAllocWidth(), buf->GetAllocHeight(),
                                    w, h, crtc_w, crtc_h);
                            }
                        }

                        HWCCHECK(eCheckDisplayCropEqualDisplayFrame);
                        if ( (fabs(w - crtc_w) > 1.0) || (fabs(h - crtc_h) > 1.0) )
                        {
                            HWCERROR(eCheckDisplayCropEqualDisplayFrame, "Plane %d %s %dx%d (alloc %dx%d) Crop %fx%f Display %dx%d",
                                plane_id, buf->IdStr(strbuf), buf->GetWidth(), buf->GetHeight(), buf->GetAllocWidth(), buf->GetAllocHeight(),
                                w, h, crtc_w, crtc_h);
                        }
                    }

                    // Set expected dimensions
                    plane->SetDisplayFrame((int32_t)crtc_x, (int32_t)crtc_y, crtc_w, crtc_h);
                    plane->SetSourceCrop(
                        float(src_x)/65536.0, float(src_y)/65536.0, w, h);
                }
                else
                {
                    plane->ClearBuf();
                }
            }
        }

        // Check the composition that created this buffer, if there was one
        mCompVal->Compare(buf);

    }
}

void DrmShimChecks::checkSetPlaneExit(int fd, uint32_t plane_id,
                                        uint32_t crtc_id,
                                        uint32_t fb_id, uint32_t flags,
                                        uint32_t crtc_x, uint32_t crtc_y,
                                        uint32_t crtc_w, uint32_t crtc_h,
                                        uint32_t src_x, uint32_t src_y,
                                        uint32_t src_w, uint32_t src_h,
                                        int ret)
{
    HWCVAL_UNUSED(fd);
    HWCVAL_UNUSED(crtc_id);
    HWCVAL_UNUSED(fb_id);
    HWCVAL_UNUSED(flags);
    HWCVAL_UNUSED(crtc_x);
    HWCVAL_UNUSED(crtc_y);
    HWCVAL_UNUSED(crtc_w);
    HWCVAL_UNUSED(crtc_h);
    HWCVAL_UNUSED(src_x);
    HWCVAL_UNUSED(src_y);
    HWCVAL_UNUSED(src_w);
    HWCVAL_UNUSED(src_h);

    HWCLOGV_COND(eLogDrm, "Enter DrmShimChecks::checkSetPlaneExit plane_id %d", plane_id);

    HWCCHECK(eCheckDrmCallSuccess);
    if (ret)
    {
        HWCERROR(eCheckDrmCallSuccess, "SetPlane failed to plane %d (status %d)", plane_id, ret);
    }

    if (mState->IsBufferMonitorEnabled())
    {
        HWCVAL_LOCK(_l,mMutex);
        mWorkQueue.Process();

        DrmShimPlane* plane = mPlanes.valueFor(plane_id);

        if (plane == 0)
        {
            return;
        }

        // Get pointer to internal CRTC object
        DrmShimCrtc* crtc = static_cast<DrmShimCrtc*>(plane->GetCrtc());

        if (crtc == 0)
        {
            return;
        }

        int32_t callDuration = plane->GetDrmCallDuration();

        if (callDuration > HWCVAL_DRM_CALL_DURATION_WARNING_LEVEL_NS)
        {
            HWCLOGW("SetPlane to plane %d took %fms", plane_id, ((double) callDuration) / 1000000.0);
        }

        // Record frame number to check for flicker
        crtc->SetDrmFrame();
    }
}

// Validation function for the drmModeSetDisplay call
// Executed immediately BEFORE the real function is called.
int DrmShimChecks::CheckSetDisplayEnter(drm_mode_set_display* drmDisp, DrmShimCrtc*& pCrtc)
{
    // Dump memory usage, if enabled
    DumpMemoryUsage();

    // Dump frame number into Kmsg (if enabled)
    HwcTestState* testState = HwcTestState::getInstance();
    ALOG_ASSERT(testState);
    if (testState->IsOptionEnabled(eOptKmsgLogging))
    {
        testState->LogToKmsg("DrmShimChecks::CheckSetDisplayEnter - frame:%d\n", mDrmFrameNo);
    }

    char strbuf[HWCVAL_DEFAULT_STRLEN];
    bool okToSet = true;
    PushThreadState ts("CheckSetDisplayEnter (locking)");

    if (!drmDisp)
    {
        return 0;
    }

    HWCVAL_LOCK(_l,mMutex);
    SetThreadState("CheckSetDisplayEnter (locked)");
    mWorkQueue.Process();

    DrmShimCrtc* crtc = mCrtcs.valueFor(drmDisp->crtc_id);
    pCrtc = crtc;

    HWCCHECK(eCheckInvalidCrtc);
    if (crtc == 0)
    {
        HWCERROR(eCheckInvalidCrtc, "drmModeSetDisplay: Invalid Crtc %d", drmDisp->crtc_id);
        return 0;
    }

    mCrcReader.CheckEnabledState(crtc);
    uint32_t framesSinceModeSet = crtc->SetDisplayEnter(mState->IsSuspended());

    // Number of (plane or pipe) scalers used.
    uint32_t scalersUsed = 0;

    if (mState->IsBufferMonitorEnabled())
    {
        if (drmDisp->update_flag & DRM_MODE_SET_DISPLAY_UPDATE_ZORDER)
        {
            uint32_t order = drmDisp->zorder & 0x7fffffff; // clear top bit which is used to define panel/HDMI
            HwcTestCrtc::SeqVector* seq = mOrders.valueFor(order);

            HWCCHECK(eCheckSetDisplayParams);
            if (seq == 0)
            {
                HWCERROR(eCheckSetDisplayParams, "drmModeSetDisplay: CRTC %d Z-order %d unknown", drmDisp->crtc_id, order);
            }
            else
            {
                HWCLOGV_COND(eLogDrm, "Setting Z-Order to %d = (%d, %d, %d)", order, (*seq)[0], (*seq)[1], (*seq)[2]);
            }

            crtc->SetZOrder(seq);
        }

        // Record frame number to check for flicker
        crtc->SetDrmFrame();

        HWCLOGD_COND(eLogDrm, "SetDisplay: size %d version %d CRTC %d Zorder %d update_flag 0x%x planes %d",
            drmDisp->size, drmDisp->version, drmDisp->crtc_id, drmDisp->zorder, drmDisp->update_flag, drmDisp->num_planes);

#ifdef DRM_PFIT_OFF
        crtc->SetPanelFitter(drmDisp->panel_fitter.mode);

        if (drmDisp->panel_fitter.mode != DRM_PFIT_OFF)
        {
            drmDisp->panel_fitter.dst_x = 0;
            drmDisp->panel_fitter.dst_y = 0;
            drmDisp->panel_fitter.dst_w = crtc->GetWidth();
            drmDisp->panel_fitter.dst_h = crtc->GetHeight();

            if (! crtc->SetPanelFitterSourceSize(drmDisp->panel_fitter.src_w, drmDisp->panel_fitter.src_h))
            {
                okToSet = false;
            }

            scalersUsed = 1;
        }
#endif

        for (uint32_t i=0; i<drmDisp->num_planes; ++i)
        {
            drm_mode_set_display_plane* drmPlane = drmDisp->plane + i;

            // 2015-05-19: Plane update is required even when the FB ID has not changed.
            // Otherwise, we get too many different errors generated when the same FB ID is used following recomposition.
            //
            // Here we rely on HWC to set the contents of the structure correctly even when the update flags are clear.
            // Fortunately they always do this, the idea is they maintain a structure and pass is the same one to DRM
            // every frame with the relevant updates.
            //
            // These ifs will be removed permanently when we are 100% sure this is OK.
            //
            //if (drmDisp->update_flag & DRM_MODE_SET_DISPLAY_UPDATE_PLANE(i))
            {
                if (drmPlane == 0 || (drmPlane->obj_id == 0))
                {
                    HWCLOGW("CheckSetDisplayEnter: null plane presented at index %d", i);
                    continue;
                }

                DrmShimPlane* plane = GetDrmPlane(drmPlane->obj_id);
                ALOG_ASSERT(plane);

                HWCLOGD_COND(eLogDrm, "SetDisplay: Plane %d upd 0x%x obj_type 0x%x FB %d tr %d src(x,y,w,h)=(%f,%f,%f,%f) crtc=(%d,%d,%d,%d) "
                    "flags 0x%x rrb2 %d alpha %d UD 0x%" PRIx64,
                    drmPlane->obj_id, drmPlane->update_flag, drmPlane->obj_type, drmPlane->fb_id, drmPlane->transform,
                    double(drmPlane->src_x)/65536.0, double(drmPlane->src_y)/65536.0,
                    double(drmPlane->src_w)/65536.0, double(drmPlane->src_h)/65536.0,
                    drmPlane->crtc_x, drmPlane->crtc_y, drmPlane->crtc_w, drmPlane->crtc_h,
                    drmPlane->flags, drmPlane->rrb2_enable, drmPlane->alpha, drmPlane->user_data);

                if (drmPlane->obj_type == DRM_MODE_OBJECT_PLANE)
                {
                    HWCCHECK(eCheckPlaneIdInvalidForCrtc);
                    if ((plane == 0) || (plane->GetCrtc() != crtc))
                    {
                        HWCERROR(eCheckPlaneIdInvalidForCrtc, "drmModeSetDisplay: Crtc %d does not have plane %d",
                            drmDisp->crtc_id, drmPlane->obj_id);
                        return 0;
                    }
                }
                else if (drmPlane->obj_type == DRM_MODE_OBJECT_CRTC)
                {
                    HWCCHECK(eCheckInvalidCrtc);
                    if ((plane == 0) || (drmPlane->obj_id != drmDisp->crtc_id))
                    {
                        HWCERROR(eCheckInvalidCrtc, "drmModeSetDisplay: Invalid Crtc %d", drmPlane->obj_id);
                        return 0;
                    }

                    if (drmPlane->update_flag & DRM_MODE_SET_DISPLAY_PLANE_UPDATE_PRESENT)
                    {
                        // Not disabled (anymore)
                        crtc->SetMainPlaneDisabled(false);
                    }
                }

                // Not a dropped frame
                crtc->IncDrawCount();

                uint32_t hwTransform;
                if (drmPlane->update_flag & DRM_MODE_SET_DISPLAY_PLANE_UPDATE_TRANSFORM)
                {
                    hwTransform = DrmTransformToHalTransform(mState->GetDeviceType(), drmPlane->transform);

                    plane->SetHwTransform(hwTransform);
                    HWCLOGD_COND(eLogDrm, "Performing transform %s on plane %d", DrmShimTransform::GetTransformName(hwTransform), drmPlane->obj_id);
                }
                else
                {
                    // Get the rotation/flip currently applied to this plane
                    hwTransform = plane->GetTransform().GetTransform();
                }

                //if (drmPlane->update_flag & DRM_MODE_SET_DISPLAY_PLANE_UPDATE_PRESENT)
                {
                    if (drmPlane->fb_id == 0)
                    {
                        plane->ClearBuf();
                    }
                    else
                    {
                        android::sp<DrmShimBuffer> buf;

                        //if (plane->GetCurrentDsId() != drmPlane->fb_id)
                        {
                            ALOG_ASSERT(crtc);
                            ALOG_ASSERT(plane);
                            buf = UpdateBufferPlane(drmPlane->fb_id, crtc, plane);

                            // Null DrmShimBuffer implies blanking.
                            if (buf.get())
                            {
                                // If the first frame since mode set, we need to ensure we have 32-bit buffer (RGBA/RGBX).
                                HWCCHECK(eCheckFirstFrame32bit);
                                if (framesSinceModeSet == 0)
                                {
                                    // if (as it should) HWC is using one of their own blanking buffers, we can't
                                    // find out the format because we only have FB ID and bo handle, we don't
                                    // have the gralloc handle.
                                    // So we just have to assume the format is OK in that case (it is likely to be)
                                    if ((buf->GetHandle() != 0) &&
                                        (buf->GetFormat() != HAL_PIXEL_FORMAT_RGBA_8888) &&
                                        (buf->GetFormat() != HAL_PIXEL_FORMAT_RGBX_8888))
                                    {
                                        if (!crtc->IsDRRSEnabled() || !crtc->IsDisplayEnabled())
                                        {
                                            HWCERROR(eCheckFirstFrame32bit, "Plane %d format %s (0x%x, 0x%x) %s",
                                                drmPlane->obj_id, FormatToStr(buf->GetFormat()), buf->GetFormat(), buf->GetDrmFormat(), buf->IdStr(strbuf));
                                        }
                                    }
                                }
                                // At this point we should check if the acquire fence has been signalled
                                // and raise eCheckDrmFence if it is not.
                                //
                                // However, we can't do this using buf->GetAcquireFenceFd() because we
                                // need to get the fence from the saved layer list, as the DrmShimBuffer
                                // does not preserve history in this respect.
                                //
                                // TODO: Think about.


                                // Check the composition that created this buffer, if there was one
                                mCompVal->Compare(buf);

                                if (mState->TestTgtImageDump(mDrmFrameNo))
                                {
                                    android::sp<android::GraphicBuffer> cpyBuf = mCompVal->CopyBuf(buf);

                                    if (cpyBuf.get())
                                    {
                                        HWCLOGI("Dumping tgt frame:%d D%d layer %d %s",
                                            mDrmFrameNo, crtc->GetDisplayIx(), i, buf->IdStr(strbuf));
                                        HwcTestDumpGrallocBufferToDisk("tgt", (1000 * mDrmFrameNo + 100 *(crtc->GetDisplayIx()) + i),
                                            cpyBuf->handle, DUMP_BUFFER_TO_TGA);
                                    }
                                    else
                                    {
                                        HWCLOGI("Failed to copy buffer %s for image dump", buf->IdStr(strbuf));
                                    }
                                }
                            }
                        }
                        /*
                        else
                        {
                            HWCLOGD_COND(eLogDrm, "Plane %d: skipped update, stays at FB %d", drmPlane->obj_id, drmPlane->fb_id);
                        }
                        */

                        if (drmPlane->obj_type == DRM_MODE_OBJECT_PLANE)
                        {
                            // check for disabling main plane
                            // TODO: check proper usage of this flag
                            if (drmPlane->flags & DRM_MODE_PAGE_FLIP_EVENT)
                            {
                                HWCLOGD("Detected callback to force main plane disabled on FB %d plane %d",drmPlane->fb_id, drmPlane->obj_id);
                                plane->GetCrtc()->SetMainPlaneDisabled(true);
                            }

                            double w = ((double) drmPlane->src_w) / 65536.0;
                            double h = ((double) drmPlane->src_h) / 65536.0;

                            if (buf.get())
                            {
                                // Don't perform checks against alloc width & height for blanking buffers
                                // as gralloc often only gives zeroes for these.
                                if (!buf->IsBlanking() && !buf->IsBlack())
                                {
                                    HWCCHECK(eCheckBufferTooSmall);
                                    if ( (w > buf->GetAllocWidth()) || (h > buf->GetAllocHeight()) )
                                    {
                                        HWCERROR(eCheckBufferTooSmall, "Plane %d %s %dx%d (alloc %dx%d) Crop %fx%f Display %dx%d",
                                            drmPlane->obj_id, buf->IdStr(strbuf), buf->GetWidth(), buf->GetHeight(), buf->GetAllocWidth(), buf->GetAllocHeight(),
                                            w, h, drmPlane->crtc_w, drmPlane->crtc_h);
                                    }
                                }

                                if (mState->GetDeviceType() == HwcTestState::eDeviceTypeBXT)
                                {
                                    scalersUsed += BroxtonPlaneValidation(crtc, buf, "Plane", drmPlane->obj_id,
                                        w, h, drmPlane->crtc_w, drmPlane->crtc_h, hwTransform);

                                    if (plane->GetPlaneIndex() >= HWCVAL_BXT_FIRST_UNSUPPORTED_NV12_PLANE)
                                    {
                                        HWCCHECK(eCheckPlaneFormatNotSupported);
                                        if (buf->IsNV12Format())
                                        {
                                            HWCERROR(eCheckPlaneFormatNotSupported,
                                                "Plane %d (index %d) does not support NV12.", plane->GetPlaneId(), plane->GetPlaneIndex());
                                        }
                                    }

#ifdef HWCVAL_ENABLE_RENDER_COMPRESSION
                                    // Perform Render Compression checks

                                    // Check that the buffer's render compression status is consistent with the plane
                                    if (buf->IsRenderCompressed() != drmPlane->render_compression)
                                    {
                                        HWCCHECK(eCheckRCNotSupportedOnPlane);
                                        HWCCHECK(eCheckRCNormalBufSentToRCPlane);
                                        if (buf->IsRenderCompressed() && !drmPlane->render_compression)
                                        {
                                            HWCERROR(eCheckRCNotSupportedOnPlane, "Buffer is marked as render compressed, but " \
                                                "plane %d (index %d) does not support Render Compression.", plane->GetPlaneId(), plane->GetPlaneIndex());
                                        }
                                        else
                                        {
                                            HWCERROR(eCheckRCNormalBufSentToRCPlane, "Non Render Compressed buffer sent to RC plane (plane id %d index %d).",
                                                plane->GetPlaneId(), plane->GetPlaneIndex());
                                        }
                                    }

                                    // Perform detailed checks
                                    if (buf->IsRenderCompressed())
                                    {
                                        // Is the format render compressible?
                                        HWCCHECK(eCheckRCInvalidFormat);
                                        if (!(buf->IsRenderCompressibleFormat()))
                                        {
                                            HWCERROR(eCheckRCInvalidFormat, "Format %d not supported with Render Compression", buf->GetFormat());
                                        }

                                        // Check the tiling. Only Y-tiling is compatible with RC.
                                        HWCCHECK(eCheckRCInvalidTiling);
                                        if (!((plane->GetTiling() != DrmShimPlane::ePlaneYTiled) || (plane->GetTiling() != DrmShimPlane::ePlaneYfTiled)))
                                        {
                                            HWCERROR(eCheckRCInvalidTiling, "Format with '%s' tiling is not supported with Render Compression",
                                                plane->GetTiling() == DrmShimPlane::ePlaneXTiled ? "X-Tiled" : "Linear");
                                        }

                                        // Check that the Aux buffer details are consistent with Gralloc
                                        HWCCHECK(eCheckRCAuxDetailsMismatch);
                                        if ((plane->GetAuxPitch() != buf->GetAuxPitch()) || (plane->GetAuxOffset() != buf->GetAuxOffset()))
                                        {
                                            HWCERROR(eCheckRCAuxDetailsMismatch, "Aux buffer details at plane (pitch: %d offset: %d) do not match Gralloc " \
                                                "(pitch: %d offset: %d)", plane->GetAuxPitch(), plane->GetAuxOffset(), buf->GetAuxPitch(), buf->GetAuxOffset());
                                        }
                                    }
#endif
                                }
                                else if (buf->IsNV12Format())
                                {
                                    HWCERROR(eCheckPlaneFormatNotSupported,
                                        "CHV/BYT planes do not support NV12.");
                                }
                                else
                                {
                                    HWCCHECK(eCheckDisplayCropEqualDisplayFrame);
                                    if ( (fabs(w - drmPlane->crtc_w) > 1.0) || (fabs(h - drmPlane->crtc_h) > 1.0) )
                                    {
                                        HWCERROR(eCheckDisplayCropEqualDisplayFrame, "Plane %d %s %dx%d (alloc %dx%d) Crop %fx%f Display %dx%d",
                                            drmPlane->obj_id, buf->IdStr(strbuf), buf->GetWidth(), buf->GetHeight(), buf->GetAllocWidth(), buf->GetAllocHeight(),
                                            w, h, drmPlane->crtc_w, drmPlane->crtc_h);
                                    }
                                }
                            }
                            else
                            {
                                HWCCHECK(eCheckDisplayCropEqualDisplayFrame);
                                if ( (fabs(w - drmPlane->crtc_w) > 1.0) || (fabs(h - drmPlane->crtc_h) > 1.0) )
                                {
                                    HWCERROR(eCheckDisplayCropEqualDisplayFrame, "Plane %d NO BUF Crop %fx%f Display %dx%d",
                                        drmPlane->obj_id,
                                        w, h, drmPlane->crtc_w, drmPlane->crtc_h);
                                }
                            }

                            // Set expected dimensions
                            plane->SetSourceCrop(
                                float(drmPlane->src_x)/65536.0,
                                float(drmPlane->src_y)/65536.0,
                                w,
                                h);
                            plane->SetDisplayFrame(drmPlane->crtc_x, drmPlane->crtc_y, drmPlane->crtc_w, drmPlane->crtc_h);

                            // Set decryption state
                            plane->SetDecrypt(drmPlane->rrb2_enable);

                            // Set blend function & plane alpha (BXT+)
                            SetPlaneBlend(plane, drmPlane);

                        }
                        else if (drmPlane->obj_type == DRM_MODE_OBJECT_CRTC)
                        {
                            if (buf.get() && (buf->GetHandle() != 0))
                            {
                                // Set expected dimensions
                                plane->SetDisplayFrame(0, 0, buf->GetWidth(), buf->GetHeight());
                                plane->SetSourceCrop(0, 0, buf->GetWidth(), buf->GetHeight());

                                // Set decryption state
                                plane->SetDecrypt(drmPlane->rrb2_enable);

                                // Set blend function & plane alpha (BXT+)
                                SetPlaneBlend(plane, drmPlane);

                                // Allocated size of buffer must be at least full screen
                                // Don't perform this check on blanking buffers, as gralloc may not
                                // get the alloc width & height
                                HWCCHECK(eCheckMainPlaneFullScreen);
                                if (!buf->IsBlanking())
                                {
                                    if ((buf->GetAllocWidth() < crtc->GetPanelFitterSourceWidth()) ||
                                         buf->GetAllocHeight() < crtc->GetPanelFitterSourceHeight())
                                    {
                                        HWCERROR(eCheckMainPlaneFullScreen, "Size is %dx%d, size to fit is %dx%d",
                                            buf->GetAllocWidth(), buf->GetAllocHeight(),
                                            crtc->GetPanelFitterSourceWidth(), crtc->GetPanelFitterSourceHeight());
                                    }
                                }
                            }
                        }

                        // Under nuclear spoofing, do not restore the page flip user data
                        if (!mState->IsOptionEnabled(eOptSpoofNuclear))

                        if ((drmPlane->obj_type == DRM_MODE_OBJECT_CRTC)
                            || (drmPlane->flags & DRM_MODE_PAGE_FLIP_EVENT))
                        {
                            if (mState->IsOptionEnabled(eOptPageFlipInterception))
                            {
                                if (drmPlane->user_data)
                                {
                                    // Setup page flip event
                                    HWCLOGD_COND(eLogEventHandler, "Crtc %d saving user data %" PRIx64, crtc->GetCrtcId(), drmPlane->user_data);
                                    crtc->SavePageFlipUserData(drmPlane->user_data);
                                    drmPlane->user_data = (__u64) crtc->GetCrtcId();
    #if !__x86_64__
                                    drmPlane->user_data &= 0xffffffff;  // try clearing high order 32 bits
    #endif
                                    HWCLOGD_COND(eLogEventHandler, "Crtc %d Page flip user data shimmed with crtc %" PRIi64, crtc->GetCrtcId(), drmPlane->user_data);
                                }
                            }
                        }

                    }
                }
            }
        }
    }

    // Broxton has only 2 scalers per pipe (1 on pipe C, which we presume isn't being used)
    // This includes panel fitter.
    if (mState->GetDeviceType() == HwcTestState::eDeviceTypeBXT)
    {
        // Pipes A and B support 2 scalers in total (plane/pipe).
        uint32_t maxScalers = 2;

        if (crtc->GetPipeIndex() == 2)
        {
            // Pipe C only supports one scaler.
            maxScalers = 1;
        }

        HWCCHECK(eCheckNumScalersUsed);
        if (scalersUsed > maxScalers)
        {
            HWCERROR(eCheckNumScalersUsed, "%d scalers used on crtc %d (pipe %d), max supported is %d",
                scalersUsed, crtc->GetCrtcId(), crtc->GetPipeIndex(), maxScalers);
        }
    }
    else
    {
        // Non-Broxton: check panel fitter not used with main plane enabled
        HWCCHECK(eCheckPanelFitterOutOfSpec);
        if (!crtc->MainPlaneIsDisabled() && drmDisp->panel_fitter.mode != DRM_PFIT_OFF)
        {
            // Panel fitter was enabled when main plane also active
            // This is unreliable, especially on CHV

            HWCERROR(eCheckPanelFitterOutOfSpec, "CRTC %d", crtc->GetCrtcId());
        }
    }


    int ret = 0;
    // Perform any display failure spoofing
    mState->GetDisplaySpoof().ModifyStatus(mDrmFrameNo, ret);
    DoStall(Hwcval::eStallSetDisplay, &mMutex);

    // We cause a setdisplay fail if either (a) we just want to test this or (b) we believe that to proceed with the SetDisplay will cause an error.
    if ((!okToSet) && mState->IsOptionEnabled(eOptBlockInvalidSetDisplay))
    {
        HWCLOGE("SetDisplay aborted because of invalid panel fitter scaling.");
        ret = -1;
    }

    HWCCHECK(eCheckNoFlipWhileDPMSDisabled);
    if (crtc->IsDPMSInProgress() || !crtc->IsDPMSEnabled())
    {
        HWCERROR(eCheckNoFlipWhileDPMSDisabled, "CRTC %d", crtc->GetCrtcId());
        ret = -1;
    }

    if (ret != 0)
    {
        // We don't expect a page flip, since there hasn't really been a SetDisplay.
        crtc->StopPageFlipWatchdog();
        crtc->SetDisplayFailed(true);
        return ret;
    }

    crtc->DrmCallStart();

    // This is to help diagnose a specific lockup condition - can be removed later.
    if (drmDisp->num_planes == 0)
    {
        HWCLOGW("Calling drmModeSetDisplay for CRTC %d with 0 planes NOW.", crtc->GetCrtcId());
    }

    return 0;
}

#ifdef HWCVAL_DRM_HAS_BLEND

enum {
    DRM_BLEND_FACTOR_AUTO,
    DRM_BLEND_FACTOR_ZERO,
    DRM_BLEND_FACTOR_ONE,
    DRM_BLEND_FACTOR_SRC_ALPHA,
    DRM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    DRM_BLEND_FACTOR_CONSTANT_ALPHA,
    DRM_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
    DRM_BLEND_FACTOR_CONSTANT_ALPHA_TIMES_SRC_ALPHA,
    DRM_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA_TIMES_SRC_ALPHA,
};

#define DRM_BLEND_FUNC(src_factor, dst_factor)        \
    (DRM_BLEND_FACTOR_##src_factor << 16 | DRM_BLEND_FACTOR_##dst_factor)

#define HWCVAL_DRM_BLEND_FUNC_NONE DRM_BLEND_FUNC(ONE, ZERO)
#define HWCVAL_DRM_BLEND_FUNC_PREMULT_PA DRM_BLEND_FUNC(CONSTANT_ALPHA, ONE_MINUS_CONSTANT_ALPHA_TIMES_SRC_ALPHA)
#define HWCVAL_DRM_BLEND_FUNC_PREMULT DRM_BLEND_FUNC(ONE, ONE_MINUS_SRC_ALPHA)
#define HWCVAL_DRM_BLEND_FUNC_COVERAGE_PA DRM_BLEND_FUNC(CONSTANT_ALPHA_TIMES_SRC_ALPHA, ONE_MINUS_CONSTANT_ALPHA_TIMES_SRC_ALPHA)
#define HWCVAL_DRM_BLEND_FUNC_COVERAGE DRM_BLEND_FUNC(SRC_ALPHA, ONE_MINUS_SRC_ALPHA)

#endif


void DrmShimChecks::SetPlaneBlend(DrmShimPlane* plane, drm_mode_set_display_plane* drmPlane)
{
    char strbuf[HWCVAL_DEFAULT_STRLEN];
#ifdef HWCVAL_DRM_HAS_BLEND
    float alpha = (drmPlane->blend_color >> (48 + 8)) / 255.0;
    bool hasPixelAlpha = true;
    Hwcval::BlendingType blend = BlendingType::NONE;

    HWCCHECK(eCheckInvalidBlend);
    switch (drmPlane->blend_func)
    {
        case HWCVAL_DRM_BLEND_FUNC_NONE:
            blend = BlendingType::NONE;
            break;

        case HWCVAL_DRM_BLEND_FUNC_PREMULT_PA:
            blend = BlendingType::PREMULTIPLIED;
            break;

        case HWCVAL_DRM_BLEND_FUNC_PREMULT:
            blend = BlendingType::PREMULTIPLIED;
            alpha = 1.0;
            break;

        case HWCVAL_DRM_BLEND_FUNC_COVERAGE_PA:
            blend = BlendingType::COVERAGE;
            break;

        case HWCVAL_DRM_BLEND_FUNC_COVERAGE:
            blend = BlendingType::COVERAGE;
            alpha = 1.0;
            break;

        default:
            HWCERROR(eCheckInvalidBlend, "Plane %d blend function 0x%" PRIx64 " colour 0x%" PRIx64,
                plane->GetPlaneId(), drmPlane->blend_func, drmPlane->blend_color);
            blend = BlendingType::PREMULTIPLIED;
    }
#else
    HWCVAL_UNUSED(drmPlane);
    float alpha = 1.0;
    bool hasPixelAlpha = plane->FormatHasPixelAlpha();
    BlendingType blend = BlendingType::PREMULTIPLIED;
#endif

    plane->GetTransform().SetBlend(blend, hasPixelAlpha, alpha);
    android::sp<DrmShimBuffer> buf = plane->GetTransform().GetBuf();
    HWCLOGV_COND(eLogDrm, "Plane %d buf %p handle %p set blend %s",
        plane->GetPlaneId(), buf.get(), buf.get() ? buf->GetHandle() : 0,
        plane->GetTransform().GetBlendingStr(strbuf));
}

uint32_t DrmShimChecks::DrmTransformToHalTransform(HwcTestState::DeviceType deviceType, uint32_t drmTransform)
{
    switch (deviceType)
    {
        case HwcTestState::eDeviceTypeCHT:
        case HwcTestState::eDeviceTypeBYT:
        {
            return drmTransform ? Hwcval::eTransformRot180 : Hwcval::eTransformNone;
        }

        default:
        {
            // Broxton and successors
            //
            // Note: the mappings for the DRM Nuclear interface are inverted in relation to the HAL transforms.
            switch (drmTransform)
            {
                case HWCVAL_DRM_ROTATE_0:
                    return Hwcval::eTransformNone;
                case HWCVAL_DRM_ROTATE_90:
                    return Hwcval::eTransformRot270;
                case HWCVAL_DRM_ROTATE_180:
                    return Hwcval::eTransformRot180;
                case HWCVAL_DRM_ROTATE_270:
                    return Hwcval::eTransformRot90;
                case HWCVAL_DRM_REFLECT_X:
                    return Hwcval::eTransformFlipH;
                case HWCVAL_DRM_REFLECT_Y:
                    return Hwcval::eTransformFlipV;
                default:
                    HWCERROR(eCheckNuclearParams, "Invalid BXT transform value %d", drmTransform);
                    return 0;
            }
        }
    }
}

static const double cdClkBxt = 288000;

// Validate any possible plane scaling against restrictions on Broxton.
// Return the number of scalers used by this plane (0 or 1).
uint32_t DrmShimChecks::BroxtonPlaneValidation(HwcTestCrtc* crtc, android::sp<DrmShimBuffer> buf, const char* str, uint32_t id,
    double srcW, double srcH,
    uint32_t dstW, uint32_t dstH,
    uint32_t transform)
{
    // If 90 degree rotation is in use then we must swap width & height of one of the co-ordinate pairs
    // At time of writing, it is TBC that this works, but it won't be great if it doesn't!
    uint32_t logDstW;
    uint32_t logDstH;

    if (transform != Hwcval::eTransformNone)
    {
        ++sHwPlaneTransformUsedCounter;
    }

    if (transform & Hwcval::eTransformRot90)
    {
        // Render compression should not be combined with 90/270 degree rotation
        HWCCHECK(eCheckRCWithInvalidRotation);
        if (buf.get() && buf->IsRenderCompressed())
        {
            HWCERROR(eCheckRCWithInvalidRotation, "Can not rotate 90/270 degrees with Render Compression");
        }

        logDstW = dstH;
        logDstH = dstW;
    }
    else
    {
        logDstW = dstW;
        logDstH = dstH;
    }
    // Per-plane scaling is enabled
    // Reference doc: 2015ww21-SScaler Validation for BXT-Android-TDS.pptx
    // This in turn is copied from the BSpec.
    //
    char strbuf[HWCVAL_DEFAULT_STRLEN];

    if ( (fabs(srcW - logDstW) > 1.0) || (fabs(srcH - logDstH) > 1.0) )
    {
        ++sHwPlaneScaleUsedCounter;
        HWCCHECK(eCheckBadScalerSourceSize);

        if ((srcW < 8) || (srcH < 8) || (srcW > 4096))
        {
            HWCERROR(eCheckBadScalerSourceSize, "%s %d %s Crop %fx%f, for BXT should be 8-4096 pixels.",
                str, id, buf.get() ? buf->IdStr(strbuf) : "",
                srcW, srcH);
        }
        else if (buf->IsVideoFormat())
        {
            if (srcH < 16)
            {
                HWCERROR(eCheckBadScalerSourceSize, "%s %d %s Crop %fx%f, for BXT min height for YUV 420 planar/NV12 formats is 16 pixels",
                    str, id, buf.get() ? buf->IdStr(strbuf) : "",
                    srcW, srcH);
            }

            // Gary says (not in the BSpec)
            if (buf->IsNV12Format())
            {
                if (srcW < 16)
                {
                    HWCERROR(eCheckBadScalerSourceSize, "%s %d %s Crop %fx%f, for BXT min width for NV12 formats is 16 pixels",
                        str, id, buf.get() ? buf->IdStr(strbuf) : "",
                        srcW, srcH);
                }
            }

        }

        // "Plane/Pipe scaling is not compatible with interlaced fetch mode."
        // HWC does not use this.

        // "Plane up and down scaling is not compatible with keying."
        // HWC does not use keying (i.e. transparent colour).

        // "Plane scaling is not compatible with the indexed 8-bit, XR_BIAS and floating point source pixel formats"
        // Not currently used in HWC.

        // Scale factor must be >1/2 for NV12, >1/3 for other formats
        double minScale = 1.0/3.0;

        if (buf->IsNV12Format())
        {
            minScale = 0.5;
        }

        double minScaleFromBandwidth = 0.0;

        if (crtc)
        {
            double crtClk = crtc->GetClock();

            if (crtClk)
            {
                minScaleFromBandwidth = crtClk / cdClkBxt;
                minScale = max(minScale, minScaleFromBandwidth);
                HWCLOGV_COND(eLogDrm, "CrtClk %f cdClkBxt %f minScaleFromBandwidth %f",
                    crtClk, cdClkBxt, minScaleFromBandwidth);
            }
            else
            {
                HWCLOGV_COND(eLogDrm, "BroxtonPlaneValidation: no crtclk for CRTC %d", crtc->GetCrtcId());
            }
        }

        double xScale = double(logDstW) / srcW;
        double yScale = double(logDstH) / srcH;
        HWCLOGV_COND(eLogDrm, "BroxtonPlaneValidation: %s %d scale %fx%f minScale %f", str, id, xScale, yScale, minScale);

        HWCCHECK(eCheckScalingFactor);
        if ((xScale <= minScale) || (yScale <= minScale) )
        {
            HWCERROR(eCheckScalingFactor, "%s %d %s %dx%d (alloc %dx%d) Crop %fx%f Display (in source frame) %dx%d Scale %fx%f",
                str, id, buf.get() ? buf->IdStr(strbuf) : "",
                buf->GetWidth(), buf->GetHeight(), buf->GetAllocWidth(), buf->GetAllocHeight(),
                srcW, srcH, logDstW, logDstH, xScale, yScale);
            HWCLOGE("  -- Minimum supported scale factor for %s is %f", buf->StrBufFormat(), minScale);
        }
        else if ((xScale * yScale) <= minScaleFromBandwidth)
        {
            HWCERROR(eCheckScalingFactor, "%s %d %s %dx%d (alloc %dx%d) Crop %fx%f Display (in source frame) %dx%d Scale %fx%f=%f",
                str, id, buf.get() ? buf->IdStr(strbuf) : "",
                buf->GetWidth(), buf->GetHeight(), buf->GetAllocWidth(), buf->GetAllocHeight(),
                srcW, srcH, logDstW, logDstH, xScale, yScale, xScale * yScale);
            HWCLOGE("  -- Minimum supported scale factor for product %s is %f", buf->StrBufFormat(), minScale);
        }

        // Scaling has been requested for this plane
        return 1;
    }
    else if (buf->IsNV12Format())
    {
        // NV12 formats use a plane scaler even when 1:1
        return 1;
    }
    else
    {
        return 0;
    }
}

// Validation function for the drmModeSetDisplay call
// Executed immediately AFTER the real function is called.
void DrmShimChecks::CheckSetDisplayExit(drm_mode_set_display* drmDisp, DrmShimCrtc* crtc, int ret)
{
    char strbuf[HWCVAL_DEFAULT_STRLEN];

    if (crtc == 0)
    {
        HWCERROR(eCheckInvalidCrtc, "No CRTC in CheckSetDisplayExit");
        return;
    }

    crtc->StopSetDisplayWatchdog();
    int32_t callDuration = crtc->GetDrmCallDuration();

    PushThreadState ts("CheckSetDisplayExit (locking)");
    HWCVAL_LOCK(_l, mMutex);
    SetThreadState("CheckSetDisplayExit (locked)");
    mWorkQueue.Process();

    HWCCHECK(eCheckDrmCallSuccess);
    if (ret)
    {
        if (mState->IsSuspended() || crtc->WasSuspended())
        {
            HWCLOGI("SetDisplay failed to crtc %d %s status %d. Expected because POWER IS SUSPENDED.",
                drmDisp->crtc_id, crtc->ReportSetDisplayPower(strbuf), ret);
        }
        else
        {
            HWCERROR(eCheckDrmCallSuccess, "SetDisplay failed to crtc %d %s status %d)",
                drmDisp->crtc_id, crtc->ReportSetDisplayPower(strbuf), ret);
        }

        // After SetDisplay failure, we know we won't get a page flip.
        crtc->StopPageFlipWatchdog();
    }

    crtc->SetDisplayFailed(ret != 0);

    if (mCurrentFrame[crtc->GetDisplayIx()] > 0)
    {
        crtc->ValidateMode(this);
    }

    if (callDuration > HWCVAL_DRM_CALL_DURATION_WARNING_LEVEL_NS)
    {
        HWCLOGW("drmModeSetDisplay to crtc %d took %fms", drmDisp->crtc_id, ((double) callDuration) / 1000000.0);
    }

    if (mState->IsBufferMonitorEnabled())
    {
        // Record frame number to check for flicker
        crtc->SetDrmFrame();

        for (uint32_t i=0; i<drmDisp->num_planes; ++i)
        {
            // 2015-05-19: Plane update is required even when the FB ID has not changed.
            // Otherwise, we get too many different errors generated when the same FB ID is used following recomposition.
            //
            // Here we rely on HWC to set the contents of the structure correctly even when the update flags are clear.
            // Fortunately they always do this, the idea is they maintain a structure and pass is the same one to DRM
            // every frame with the relevant updates.
            //
            // These ifs will be removed permanently when we are 100% sure this is OK.
            //
            // if (drmDisp->update_flag & DRM_MODE_SET_DISPLAY_UPDATE_PLANE(i))
            {
                drm_mode_set_display_plane* drmPlane = drmDisp->plane + i;
                DrmShimPlane* plane = GetDrmPlane(drmPlane->obj_id);

                if (plane == 0)
                {
                    // No such plane. Check it is disabled.
                    ALOG_ASSERT(drmPlane->fb_id == 0);
                    continue;
                }

                plane->SetDisplayFailed(ret != 0);

                if (drmPlane->obj_type == DRM_MODE_OBJECT_PLANE)
                {
                    if (plane->GetCrtc() != crtc)
                    {
                        return;
                    }
                }
                else if (drmPlane->obj_type == DRM_MODE_OBJECT_CRTC)
                {
                    if (drmPlane->obj_id != drmDisp->crtc_id)
                    {
                        return;
                    }
                }

                // if (drmPlane->update_flag & DRM_MODE_SET_DISPLAY_PLANE_UPDATE_PRESENT)

                // Under nuclear spoofing, no need to restore the page flip user data
                if (!mState->IsOptionEnabled(eOptSpoofNuclear))
                {
                    if (drmPlane->fb_id != 0)
                    {
                        if ((drmPlane->obj_type == DRM_MODE_OBJECT_CRTC)
                            || (drmPlane->flags & DRM_MODE_PAGE_FLIP_EVENT))
                        {
                            if (mState->IsOptionEnabled(eOptPageFlipInterception))
                            {
                                if (drmPlane->user_data)
                                {
                                    // Restore page flip user data ready for next time
                                    drmPlane->user_data = crtc->GetPageFlipUserData();
                                    HWCLOGD_COND(eLogEventHandler, "Crtc %d Page flip user data restored to %llx", crtc->GetCrtcId(), drmPlane->user_data);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Dump memory usage, if enabled
    DumpMemoryUsage();
}

#ifdef DRM_IOCTL_MODE_ATOMIC

bool DrmShimChecks::ConvertToSetDisplay(struct drm_mode_atomic* drmAtomic, drm_mode_set_display*& drmDisp, DrmShimCrtc::DrmModeAddFB2Func addFb2Func)
{
    PushThreadState ts("ConvertToSetDisplay (locking)");
    HWCVAL_LOCK(_l, mMutex);
    SetThreadState("ConvertToSetDisplay (locked)");
    mWorkQueue.Process();

    HWCLOGD_COND(eLogNuclear, "DrmShimChecks::ConvertToSetDisplay atomic=%p", drmAtomic);
    ALOG_ASSERT(mPropMgr);

    // TODO drmAtomic->flags: I think I can ignore these because
    // 1. TEST_ONLY is not interesting
    // 2. NONBLOCK: we can't do this, and HWC doesn't need it.
    // 3. ALLOW_MODESET: HWC will not support this.

    uint32_t* pObjs = (uint32_t*) drmAtomic->objs_ptr;
    uint32_t* pCountProps = (uint32_t*) drmAtomic->count_props_ptr;
    uint32_t* pProp = (uint32_t*) drmAtomic->props_ptr;
    uint64_t* pPropValue = (uint64_t*) drmAtomic->prop_values_ptr;
    uint32_t displayIx = HWCVAL_MAX_CRTCS;
    uint32_t crtcId = 0;

    for (uint32_t i=0; i<drmAtomic->count_objs; ++i)
    {
        uint32_t obj_id = pObjs[i];
        uint32_t countProps = pCountProps[i];

        HWCLOGV_COND(eLogNuclear, "DrmShimChecks::ConvertToSetDisplay object %d has %d properties", obj_id, countProps);

        // Search for destination plane in the setdisplay structure
        DrmShimPlane* plane = mPlanes.valueFor(obj_id);
        if (plane == 0)
        {
            HWCERROR(eCheckPlaneIdInvalidForCrtc, "Nuclear: No such plane %d", obj_id);
            return false;
        }

        DrmShimCrtc* crtc = static_cast<DrmShimCrtc*>(plane->GetCrtc());
        crtcId = crtc->GetCrtcId();
        uint32_t d=crtc->GetDisplayIx();
        uint32_t planeId = obj_id;
        crtc->SavePageFlipUserData(drmAtomic->user_data);

        // For now, require each nuclear call to contain only information about one display
        if (i>0)
        {
            ALOG_ASSERT( d == displayIx);
        }
        else
        {
            displayIx = d;

            drmDisp = mSetDisplay + d;

            if (drmDisp->crtc_id == 0)
            {
                // First time for this display, initialize everything
                drmDisp->crtc_id = crtcId;
                drmDisp->num_planes = crtc->NumPlanes();
                HWCLOGV_COND(eLogNuclear, "DrmShimChecks::ConvertToSetDisplay D%d is crtc %d", d, drmDisp->crtc_id);

                // Update everything first time
                drmDisp->update_flag = DRM_MODE_SET_DISPLAY_PLANE_UPDATE_PRESENT |
                    DRM_MODE_SET_DISPLAY_UPDATE_ZORDER;

                drmDisp->version = DRM_MODE_SET_DISPLAY_VERSION;
                drmDisp->size = sizeof(drm_mode_set_display);

                // Force panel fitter update (PFIT:OFF).
                drmDisp->update_flag |= DRM_MODE_SET_DISPLAY_UPDATE_PANEL_FITTER;
                drmDisp->panel_fitter.mode = DRM_PFIT_OFF;
                drmDisp->panel_fitter.src_w = crtc->GetWidth();
                drmDisp->panel_fitter.src_h = crtc->GetHeight();
                drmDisp->panel_fitter.dst_w = crtc->GetWidth();
                drmDisp->panel_fitter.dst_h = crtc->GetHeight();

                ALOG_ASSERT(crtc->NumPlanes() <= DRM_MODE_SET_DISPLAY_MAX_PLANES);
                HWCLOGD_COND(eLogNuclear, "DrmShimChecks::ConvertToSetDisplay CRTC %d numPlanes %d", crtcId, drmDisp->num_planes);

                // Set initial state for the planes in the setdisplay buffer
                for (uint32_t pl=0; pl<crtc->NumPlanes(); ++pl)
                {
                    drm_mode_set_display_plane* drmPlane = drmDisp->plane + pl;
                    DrmShimPlane* plane = crtc->GetPlane(pl);
                    drmPlane->obj_id = plane->GetDrmPlaneId();
                    drmPlane->fb_id = -1;

                    // If we have substituted the main plane id with the crtc id, in order to
                    // keep older (non-nuclear) platforms happy, we need to change the object
                    // type to DRM_MODE_OBJECT_PLANE.
                    drmPlane->obj_type = (plane->GetCrtc()->GetCrtcId() == drmPlane->obj_id)
                        ? DRM_MODE_OBJECT_CRTC : DRM_MODE_OBJECT_PLANE;

                    // Force Plane update (to disabled).
                    drmDisp->update_flag |= DRM_MODE_SET_DISPLAY_UPDATE_PLANE(pl);
                    drmPlane->update_flag |= DRM_MODE_SET_DISPLAY_PLANE_UPDATE_PRESENT;
                    drmPlane->transform = HWCVAL_DRM_ROTATE_0;

                    // Initialise render compression and blend
#ifdef HWCVAL_ENABLE_RENDER_COMPRESSION
                    drmPlane->render_compression = 0;
#endif

// Extension for Broxton which supports different blend options.
#ifdef HWCVAL_DRM_HAS_BLEND
                    // Default is PREMULT with 100% plane alpha
                    drmPlane->blend_func = HWCVAL_DRM_BLEND_FUNC_PREMULT_PA;
                    drmPlane->blend_color = uint64_t(255) << (48 + 8);
#endif

                }
            }
            else
            {
                drmDisp->update_flag = 0;

                // Clear plane flags
                for (uint32_t pl=0; pl<DRM_MODE_SET_DISPLAY_MAX_PLANES; ++pl)
                {
                    drm_mode_set_display_plane* drmPlane = drmDisp->plane + pl;
                    drmPlane->flags = 0;
                }
            }
        }

        drm_mode_set_display_plane* drmPlane = 0;
        uint32_t drmPlaneIx = 0;

        for (uint32_t pl=0; pl<DRM_MODE_SET_DISPLAY_MAX_PLANES; ++pl)
        {
            uint32_t sdPlaneId = drmDisp->plane[pl].obj_id;

            if (sdPlaneId == plane->GetDrmPlaneId())
            {
                drmPlane = drmDisp->plane + pl;

                // Mark the plane as updated
                drmDisp->update_flag |= DRM_MODE_SET_DISPLAY_UPDATE_PLANE(pl);
                drmPlaneIx = pl;

                break;
            }
        }

        HWCCHECK(eCheckPlaneIdInvalidForCrtc);
        if (drmPlane == 0)
        {
            HWCERROR(eCheckPlaneIdInvalidForCrtc, "Nuclear: CRTC %d does not have plane %d", crtcId, planeId);
            continue;
        }

        // Put the user data on all planes. Flags will indicate if they are important.
        drmPlane->update_flag = 0;

        for (uint32_t j=0; j<countProps; ++j)
        {
            uint32_t prop = *pProp++;
            uint64_t value = *pPropValue++;

            HwcTestKernel::ObjectClass propClass = HwcTestKernel::eCrtc;
            Hwcval::PropertyManager::PropType propType = mPropMgr->PropIdToType(prop, propClass);

            HWCLOGV_COND(eLogNuclear, "Property %s value %" PRIi64, mPropMgr->GetName(propType), value);

            HWCCHECK(eCheckIoctlParameters);
            if (propClass != ePlane)
            {
                HWCERROR(eCheckIoctlParameters, "Nuclear: Plane %d has CRTC property 0x%" PRIx64 " specified",
                    planeId, prop);
                continue;
            }

            switch (propType)
            {
                case Hwcval::PropertyManager::eDrmPlaneProp_SRC_X:
                {
                    drmPlane->src_x = value;
                    drmPlane->update_flag |= DRM_MODE_SET_DISPLAY_PLANE_UPDATE_PRESENT;
                    break;
                }
                case Hwcval::PropertyManager::eDrmPlaneProp_SRC_Y:
                {
                    drmPlane->src_y = value;
                    drmPlane->update_flag |= DRM_MODE_SET_DISPLAY_PLANE_UPDATE_PRESENT;
                    break;
                }
                case Hwcval::PropertyManager::eDrmPlaneProp_SRC_W:
                {
                    drmPlane->src_w = value;
                    drmPlane->update_flag |= DRM_MODE_SET_DISPLAY_PLANE_UPDATE_PRESENT;
                    break;
                }
                case Hwcval::PropertyManager::eDrmPlaneProp_SRC_H:
                {
                    drmPlane->src_h = value;
                    drmPlane->update_flag |= DRM_MODE_SET_DISPLAY_PLANE_UPDATE_PRESENT;
                    break;
                }
                case Hwcval::PropertyManager::eDrmPlaneProp_CRTC_X:
                {
                    drmPlane->crtc_x = value;
                    drmPlane->update_flag |= DRM_MODE_SET_DISPLAY_PLANE_UPDATE_PRESENT;
                    break;
                }
                case Hwcval::PropertyManager::eDrmPlaneProp_CRTC_Y:
                {
                    drmPlane->crtc_y = value;
                    drmPlane->update_flag |= DRM_MODE_SET_DISPLAY_PLANE_UPDATE_PRESENT;
                    break;
                }
                case Hwcval::PropertyManager::eDrmPlaneProp_CRTC_W:
                {
                    drmPlane->crtc_w = value;
                    drmPlane->update_flag |= DRM_MODE_SET_DISPLAY_PLANE_UPDATE_PRESENT;
                    break;
                }
                case Hwcval::PropertyManager::eDrmPlaneProp_CRTC_H:
                {
                    drmPlane->crtc_h = value;
                    drmPlane->update_flag |= DRM_MODE_SET_DISPLAY_PLANE_UPDATE_PRESENT;
                    break;
                }
                case Hwcval::PropertyManager::eDrmPlaneProp_CRTC_ID:
                {
                    HWCCHECK(eCheckPlaneIdInvalidForCrtc);
                    if (value == 0)
                    {
                        drmPlane->user_data = 0;
                        drmPlane->flags = 0;
                    }
                    else if (value != crtcId)
                    {
                        HWCERROR(eCheckPlaneIdInvalidForCrtc, "Nuclear: Plane %d does not belong to CRTC %d",
                            obj_id, value);
                    }
                    break;
                }
                case Hwcval::PropertyManager::eDrmPlaneProp_FB_ID:
                {
                    if (value != 0)
                    {
                        if (drmPlane->fb_id == 0)
                        {
                            // Force an update all properties when a plane transitions from disabled->enabled.
                            drmPlane->update_flag |= ( DRM_MODE_SET_DISPLAY_PLANE_UPDATE_ALPHA
                                                   | DRM_MODE_SET_DISPLAY_PLANE_UPDATE_RRB2
                                                   | DRM_MODE_SET_DISPLAY_PLANE_UPDATE_TRANSFORM );
                            drmPlane->alpha = 1;
                        }

                        drmPlane->fb_id = value;
                        drmPlane->update_flag |= DRM_MODE_SET_DISPLAY_PLANE_UPDATE_PRESENT;
                    }
                    else
                    {
                        if (drmPlane->fb_id != 0)
                        {
                            // Plane is disabled.
                            // Matches behaviour of HWC SetDisplayPageFlipHandler - plane attributes
                            // do not persist after this.
                            if (drmPlane->obj_type == DRM_MODE_OBJECT_CRTC)
                            {
                                if (crtc->MainPlaneIsDisabled())
                                {
                                    HWCLOGD_COND(eLogNuclear, "Nuclear: Main plane CRTC %d already disabled.", drmPlane->obj_id);
                                    break;
                                }
                                else
                                {
                                    HWCLOGD_COND(eLogNuclear, "Nuclear: Disabling main plane CRTC %d", drmPlane->obj_id);
                                }

                                // Under SetDisplay, if main plane is turned off it needs a blanking buffer.
                                drmPlane->fb_id = crtc->GetBlankingFb(mGralloc, addFb2Func, mShimDrmFd);
                                ALOG_ASSERT(drmPlane->fb_id);
                                drmPlane->crtc_w = crtc->GetWidth();
                                drmPlane->crtc_h = crtc->GetHeight();

                                mWorkQueue.Process();
                            }
                            else
                            {
                                drmPlane->fb_id   = 0;
                                drmPlane->crtc_w  = 0;
                                drmPlane->crtc_h  = 0;
                            }

                            drmPlane->crtc_x      = 0;
                            drmPlane->crtc_y      = 0;
                            drmPlane->src_x       = 0;
                            drmPlane->src_y       = 0;
                            drmPlane->src_w       = 0;
                            drmPlane->src_h       = 0;
                            drmPlane->user_data   = 0;
                            drmPlane->flags       = 0;
                            drmPlane->alpha       = 0;
                            drmPlane->rrb2_enable = 0;
                            drmPlane->transform   = HWCVAL_DRM_ROTATE_0;

                            drmPlane->update_flag |= DRM_MODE_SET_DISPLAY_PLANE_UPDATE_PRESENT;
                        }
                        else
                        {
                            // Plane remains turned off. This is not an update.
                            drmDisp->update_flag &= ~DRM_MODE_SET_DISPLAY_UPDATE_PLANE(drmPlaneIx);
                        }
                    }

                    break;
                }
                case Hwcval::PropertyManager::eDrmPlaneProp_RRB2:
                {
                    drmPlane->rrb2_enable = value;
                    drmPlane->update_flag |= DRM_MODE_SET_DISPLAY_PLANE_UPDATE_RRB2;
                    break;
                }
                case Hwcval::PropertyManager::eDrmPlaneProp_rotation:
                {
                    if ((mState->GetDeviceType() == HwcTestState::eDeviceTypeBYT) ||
                        (mState->GetDeviceType() == HwcTestState::eDeviceTypeCHT))
                    {
                        // Legacy platforms only support 0 and 180 degrees
                        HWCCHECK(eCheckNuclearParams);
                        switch (value)
                        {
                            case 0:
                            case HWCVAL_DRM_ROTATE_0:
                            case HWCVAL_DRM_ROTATE_180:
                                break;
                            default:
                                HWCERROR(eCheckNuclearParams, "Nuclear: Crtc %d Plane %d transform value %d invalid", crtcId, obj_id, value);
                        }
                    }

                    // Broxton and future platforms support all transforms
                    drmPlane->transform = value;
                    drmPlane->update_flag |= DRM_MODE_SET_DISPLAY_PLANE_UPDATE_TRANSFORM;
                    break;
                }
                case Hwcval::PropertyManager::eDrmPlaneProp_render_compression:
                {
                    HWCLOGD_COND(eLogNuclear, "Nuclear: Setting Render Compression field on plane %d (crtc %d) to %d", drmPlaneIx, crtcId, value);
#ifdef HWCVAL_ENABLE_RENDER_COMPRESSION
                    drmPlane->render_compression = value;
#endif
                    break;
                }
                case Hwcval::PropertyManager::eDrmPlaneProp_blend_func:
                {
                    drmPlane->blend_func = value;
                    break;
                }
                case Hwcval::PropertyManager::eDrmPlaneProp_blend_color:
                {
                    drmPlane->blend_color = value;
                    break;
                }
                case Hwcval::PropertyManager::eDrmPlaneProp_I915_COLOR_MATRIX:
                {
                    // TODO
                    break;
                }

                // Display properties...
                case Hwcval::PropertyManager::eDrmCrtcProp_I915_PF_X:
                {
                    // SetDisplay does not have this attribute - not currently present
                    // on panel fitter.
                    //drmDisp->panel_fitter.src_x = value;
                    drmDisp->update_flag |= DRM_MODE_SET_DISPLAY_UPDATE_PANEL_FITTER;
                    break;
                }
                case Hwcval::PropertyManager::eDrmCrtcProp_I915_PF_Y:
                {
                    // SetDisplay does not have this attribute - not currently present
                    // on panel fitter.
                    //drmDisp->panel_fitter.src_y = value;
                    drmDisp->update_flag |= DRM_MODE_SET_DISPLAY_UPDATE_PANEL_FITTER;
                    break;
                }
                case Hwcval::PropertyManager::eDrmCrtcProp_I915_PF_W:
                {
                    drmDisp->panel_fitter.src_w = value;
                    drmDisp->update_flag |= DRM_MODE_SET_DISPLAY_UPDATE_PANEL_FITTER;
                    break;
                }
                case Hwcval::PropertyManager::eDrmCrtcProp_I915_PF_H:
                {
                    drmDisp->panel_fitter.src_h = value;
                    drmDisp->update_flag |= DRM_MODE_SET_DISPLAY_UPDATE_PANEL_FITTER;
                    break;
                }
                case Hwcval::PropertyManager::eDrmCrtcProp_I915_PF_MODE:
                {
                    drmDisp->panel_fitter.mode = value;
                    drmDisp->update_flag |= DRM_MODE_SET_DISPLAY_UPDATE_PANEL_FITTER;
                    break;
                }
                case Hwcval::PropertyManager::eDrmCrtcProp_I915_ZORDER:
                {
                    drmDisp->zorder = value;
                    drmDisp->update_flag |= DRM_MODE_SET_DISPLAY_UPDATE_ZORDER;
                    break;
                }
                case Hwcval::PropertyManager::eDrmCrtcProp_I915_REFRESH:
                {
                    // TODO. Not present in struct drm_mode_set_display.
                    break;
                }

                default:
                {
                    HWCERROR(eCheckIoctlParameters, "Nuclear: Plane %d unsupported property %d", obj_id, prop);
                }
            }
        }
    }

    // Set the page flip flag on the first enabled plane
    bool pageFlipFlagSet = false;
    for (uint32_t pl=0; pl<DRM_MODE_SET_DISPLAY_MAX_PLANES; ++pl)
    {
        drm_mode_set_display_plane* drmPlane = drmDisp->plane + pl;

        if ((drmPlane->fb_id != 0) && !pageFlipFlagSet)
        {
            if (mState->IsOptionEnabled(eOptSpoofNuclear))
            {
                HWCLOGD_COND(eLogNuclear, "Nuclear: crtc %d: setting plane [%d] user_data to validation value: %d", crtcId, pl, crtcId);
                drmPlane->user_data = crtcId;
            }
            else
            {
                HWCLOGD_COND(eLogNuclear, "Nuclear: crtc %d: setting plane [%d] user_data to %p", crtcId, pl, drmAtomic->user_data);
                drmPlane->user_data = drmAtomic->user_data;
            }

            drmPlane->flags |= DRM_MODE_PAGE_FLIP_EVENT;
            pageFlipFlagSet = true;
        }
        else
        {
            drmPlane->user_data = 0;
            drmPlane->flags &= ~DRM_MODE_PAGE_FLIP_EVENT;
        }
    }

    ALOG_ASSERT(drmDisp->crtc_id == crtcId);

    return true;
}

EXPORT_API void DrmShimChecks::AtomicShimUserData(struct drm_mode_atomic* drmAtomic)
{
    uint32_t* pObjs = (uint32_t*) drmAtomic->objs_ptr;

    for (uint32_t i=0; i<drmAtomic->count_objs; ++i)
    {
        uint32_t obj_id = pObjs[i];

        // Search for destination plane in the setdisplay structure
        DrmShimPlane* plane = mPlanes.valueFor(obj_id);
        if (plane == 0)
        {
            continue;
        }

        if (mState->IsOptionEnabled(eOptPageFlipInterception))
        {
            DrmShimCrtc* crtc = static_cast<DrmShimCrtc*>(plane->GetCrtc());
            crtc->SavePageFlipUserData(drmAtomic->user_data);
            drmAtomic->user_data = (__u64) crtc->GetCrtcId();
#if !__x86_64__
            drmAtomic->user_data &= 0xffffffff;  // try clearing high order 32 bits
#endif
            HWCLOGD_COND(eLogEventHandler, "Crtc %d Nuclear:Page flip user data shimmed with crtc %llx", crtc->GetCrtcId(), drmAtomic->user_data);
        }
        break;
    }
}

EXPORT_API void DrmShimChecks::AtomicUnshimUserData(struct drm_mode_atomic* drmAtomic)
{
    uint32_t* pObjs = (uint32_t*) drmAtomic->objs_ptr;

    for (uint32_t i=0; i<drmAtomic->count_objs; ++i)
    {
        uint32_t obj_id = pObjs[i];

        // Search for destination plane in the setdisplay structure
        DrmShimPlane* plane = mPlanes.valueFor(obj_id);
        if (plane == 0)
        {
            continue;
        }

        if (mState->IsOptionEnabled(eOptPageFlipInterception))
        {
            DrmShimCrtc* crtc = static_cast<DrmShimCrtc*>(plane->GetCrtc());
            if (drmAtomic->user_data)
            {
                // Restore page flip user data ready for next time
                drmAtomic->user_data = crtc->GetPageFlipUserData();
                HWCLOGD_COND(eLogEventHandler, "Crtc %d Nuclear: Page flip user data restored to %llx", crtc->GetCrtcId(), drmAtomic->user_data);
            }
        }
        break;
    }
}

#endif // DRM_IOCTL_MODE_ATOMIC

void DrmShimChecks::CheckIoctlI915SetPlaneZOrder(struct drm_i915_set_plane_zorder* ord)
{
#if HWCVAL_HAVE_ZORDER_API
    HWCVAL_LOCK(_l,mMutex);
    mWorkQueue.Process();

    uint32_t order = ord->order;
    uint32_t displayIx;

    if ((order & (1<<31)) == 0)
    {
        // Panel
        displayIx = eDisplayIxFixed;
    }
    else
    {
        displayIx = eDisplayIxHdmi;
    }

    order &= 0x7fffffff; // clear top bit which is used to define panel/HDMI
    DrmShimCrtc* crtc = GetCrtcByDisplayIx(displayIx);
    uint32_t crtcId = crtc->GetCrtcId();

    HwcTestCrtc::SeqVector* seq = mOrders.valueFor(order);
    HWCLOGD_COND(eLogDrm, "Setting CRTC %d Z-Order to %d = (%d, %d, %d)", crtcId, order, (*seq)[0], (*seq)[1], (*seq)[2]);

    HWCCHECK(eCheckIoctlParameters);
    if (seq == 0)
    {
        HWCERROR(eCheckIoctlParameters, "CRTC %d Z-order %d unknown", crtcId, order);
    }

    crtc->SetZOrder(seq);
    crtc->ClearMaxFifo();

    // Flicker detection
    crtc->SetDrmFrame();
#else
    HWCLOGD("Plane ZOrder support missing");
#endif
}

void DrmShimChecks::CheckIoctlI915SetPlane180Rotation(struct drm_i915_plane_180_rotation* rot)
{
    if (rot->obj_type == DRM_MODE_OBJECT_PLANE || rot->obj_type == DRM_MODE_OBJECT_CRTC)
    {
        HWCVAL_LOCK(_l,mMutex);
        mWorkQueue.Process();

        DrmShimPlane* plane = mPlanes.valueFor(rot->obj_id);

        if (plane)
        {
            uint32_t hwTransform = rot->rotate ? Hwcval::eTransformRot180 : Hwcval::eTransformNone;
            plane->SetHwTransform(hwTransform);
            HWCLOGD("Performing transform %s on plane %d", DrmShimTransform::GetTransformName(hwTransform), rot->obj_id);

            // Redraw is necessary after hardware transform
            plane->SetRedrawExpected(true);

            // Flicker detection
            DrmShimCrtc* crtc = static_cast<DrmShimCrtc*>(plane->GetCrtc());

            if (crtc)
            {
                crtc->SetDrmFrame();
            }
        }
        else
        {
            HWCERROR(eCheckIoctlParameters, "SetPlane180Rotation: plane %d unknown", rot->obj_id);
        }
        HWCCHECK(eCheckIoctlParameters);
    }
}

#if BUILD_I915_DISP_SCREEN_CONTROL
void DrmShimChecks::CheckIoctlI915DispScreenControl(struct drm_i915_disp_screen_control* ctrl, int status)
{
    HWCVAL_LOCK(_l,mMutex);
    mWorkQueue.Process();

    // This ALWAYS fails on CHV with no side effects, so no point in doing the check.
    //if (status != 0)
    //{
    //    HWCERROR(eCheckDrmCallSuccess, "DRM_IOCTL_I915_DISP_SCREEN_CONTROL failed crtc %d status %d",
    //        ctrl->crtc_id, status);
    //}

    if (status == 0)
    {
        DrmShimCrtc* crtc = mCrtcs.valueFor(ctrl->crtc_id);
        HWCLOGI("DispScreenControl: crtc %d @ %p enable %d", ctrl->crtc_id, crtc, ctrl->on_off_cntrl);

        if (crtc)
        {
            crtc->SetDisplayEnable(ctrl->on_off_cntrl);
        }
    }
    else
    {
        HWCLOGI("DispScreenControl: crtc %d enable %d FAILED", ctrl->crtc_id, ctrl->on_off_cntrl);
    }
}
#endif

void DrmShimChecks::CheckIoctlI915SetDecrypt(struct drm_i915_reserved_reg_bit_2* decrypt)
{
    if (decrypt)
    {
        uint32_t planeId;
#ifdef INTEL_HWC_ANDROID_MCG
        planeId = decrypt->plane + 2;
#else
        planeId = decrypt->plane;
#endif
        HWCLOGD("CheckIoctlI915SetDecrypt: Plane %d enable %d", planeId, decrypt->enable);
        HWCVAL_LOCK(_l,mMutex);
        mWorkQueue.Process();

        DrmShimPlane* plane = mPlanes.valueFor(planeId);

        if (plane)
        {
            if (plane->IsMainPlane())
            {
                HWCCHECK(eCheckPavpOverlayPlane);
                if (decrypt->enable)
                {
                    // TODO: Consider future platforms where this may not be the case.
                    HWCERROR(eCheckPavpOverlayPlane, "Main plane %d does not support encryption", planeId);
                }
            }
            else
            {
                plane->SetDecrypt(decrypt->enable);
            }
        }
    }
}

uint32_t DrmShimChecks::GetCrtcIdForConnector(uint32_t conn_id)
{
    DrmShimCrtc* crtc = mConnectors.valueFor(conn_id).mCrtc;

    if (crtc)
    {
        return crtc->GetCrtcId();
    }
    else
    {
        return 0;
    }
}

void DrmShimChecks::CheckSetDPMS(uint32_t conn_id, uint64_t value, HwcTestEventHandler* eventHandler, HwcTestCrtc*& theCrtc, bool& reenable)
{
    PushThreadState ts("CheckSetDPMS (locking)");
    HWCVAL_LOCK(_l,mMutex);
    SetThreadState("CheckSetDPMS (locked)");
    mWorkQueue.Process();

    DrmShimCrtc* crtc = mConnectors.valueFor(conn_id).mCrtc;
    theCrtc = crtc;
    uint32_t crtcId = (crtc ? crtc->GetCrtcId() : 0);
    HWCLOGD("CheckSetDPMS conn_id=%d crtc %d value=%d", conn_id, crtcId, value);

    if (crtc)
    {
        // Panel was turned on or off
        if (value == DRM_MODE_DPMS_OFF)
        {
            crtc->SetDPMSEnabled(false);
            mCrcReader.SuspendCRCs(crtc->GetCrtcId(), HwcCrcReader::CRC_SUSPEND_BLANKING, true);
        }
#ifdef DRM_MODE_DPMS_ASYNC_OFF
        else if (value == DRM_MODE_DPMS_ASYNC_OFF)
        {
            crtc->SetDPMSEnabled(false); // TODO: how is async off different?
                                         // though hopefully it doesn't matter given the 4 frame window
            mCrcReader.SuspendCRCs(crtc->GetCrtcId(), HwcCrcReader::CRC_SUSPEND_BLANKING, true);
        }
#endif
        else if (value == DRM_MODE_DPMS_ON)
        {
            crtc->SetDPMSEnabled(true);
            mCrcReader.SuspendCRCs(crtc->GetCrtcId(), HwcCrcReader::CRC_SUSPEND_BLANKING, false);
        }
#ifdef DRM_MODE_DPMS_ASYNC_ON
        else if (value == DRM_MODE_DPMS_ASYNC_ON)
        {
            crtc->SetDPMSEnabled(true);
            mCrcReader.SuspendCRCs(crtc->GetCrtcId(), HwcCrcReader::CRC_SUSPEND_BLANKING, false);
        }
#endif

        if ((crtcId > 0) && (eventHandler != 0))
        {
            eventHandler->CancelEvent(crtcId);

            if (value == DRM_MODE_DPMS_ON
#ifdef DRM_MODE_DPMS_ASYNC_ON
             || (value == DRM_MODE_DPMS_ASYNC_ON)
#endif
                )
            {
                if (crtc)
                {
                    bool reenableVBlank = crtc->IsModeSet();
                    reenable = reenableVBlank;
                }
            }
            else
            {
                if (crtc)
                {
                    crtc->SetModeSet(false);
                }
            }
        }

        DoStall(Hwcval::eStallDPMS, &mMutex);
        crtc->SetDPMSInProgress(true);
    }
    else
    {
        HWCLOGW("DPMS Enable/disable for unknown connector %d",conn_id);
    }
}

void DrmShimChecks::CheckSetDPMSExit(uint32_t fd, HwcTestCrtc* crtc, bool reenable, HwcTestEventHandler* eventHandler, uint32_t status)
{
    HWCVAL_UNUSED(status);
    HWCLOGD("CheckSetDPMSExit crtc %d status=%d", crtc->GetCrtcId(), status);

    PushThreadState ts("CheckSetDPMSExit (locking)");
    HWCVAL_LOCK(_l,mMutex);
    SetThreadState("CheckSetDPMSExit (locked)");
    mWorkQueue.Process();

    if (crtc)
    {
        crtc->SetDPMSInProgress(false);

        if (reenable && eventHandler)
        {
            eventHandler->CaptureVBlank(fd, crtc->GetCrtcId());
        }
    }
}

void DrmShimChecks::CheckSetPanelFitter(uint32_t conn_id, uint64_t value)
{
    HWCLOGD("CheckSetPanelFitter conn_id=%d value=%d", conn_id, value);

    PushThreadState ts("CheckSetPanelFitter (locking)");
    HWCVAL_LOCK(_l,mMutex);
    SetThreadState("CheckSetPanelFitter (locked)");
    mWorkQueue.Process();

    DrmShimCrtc* crtc = mConnectors.valueFor(conn_id).mCrtc;

    if (crtc)
    {
        crtc->SetPanelFitter(value);
    }
    else
    {
        HWCLOGW("SetPanelFitter for unknown connector %d",conn_id);
    }
}

void DrmShimChecks::CheckSetPanelFitterSourceSize(uint32_t conn_id, uint32_t sw, uint32_t sh)
{
    HWCLOGD("CheckSetPanelFitterSourceSize conn_id=%d sw=%d, sh=%d", conn_id, sw, sh);

    PushThreadState ts("CheckPanelFitterSourceSize (locking)");
    HWCVAL_LOCK(_l,mMutex);
    SetThreadState("CheckPanelFitterSourceSize (locked)");
    mWorkQueue.Process();

    DrmShimCrtc* crtc = mConnectors.valueFor(conn_id).mCrtc;

    if (crtc)
    {
        crtc->SetPanelFitterSourceSize(sw, sh);
    }
    else
    {
        // TODO: understand why this happens sometimes.
        // Otherwise this really should be HWCERROR(eCheckIoctlParameters,...
        HWCLOGW("SetPanelFitterSourceSize for unknown connector %d",conn_id);
    }
}

android::sp<DrmShimBuffer> DrmShimChecks::UpdateBufferPlane(uint32_t fbId, DrmShimCrtc* crtc, DrmShimPlane* plane)
{
    mWorkQueue.Process();

    ssize_t ix = mBuffersByFbId.indexOfKey(fbId);
    android::sp<DrmShimBuffer> buf;
    char strbuf[HWCVAL_DEFAULT_STRLEN];

    plane->SetCurrentDsId((int64_t)fbId);

    if (ix >= 0)
    {
        // Buffer already known to us
        // Just record that we want it drawn, but it isn't new
        buf = mBuffersByFbId.valueAt(ix);
        ALOG_ASSERT(buf.get());

        DrmShimBuffer::FbIdData *pFbIdData = buf->GetFbIdData(fbId);
        uint32_t pixelFormat = pFbIdData ? pFbIdData->pixelFormat : 0;
        bool hasAuxBuffer = pFbIdData ? pFbIdData->hasAuxBuffer : false;
        uint32_t auxPitch = pFbIdData ? pFbIdData->auxPitch : 0;
        uint32_t auxOffset = pFbIdData ? pFbIdData->auxOffset : 0;
        __u64 modifier = pFbIdData ? pFbIdData->modifier : 0;

        plane->SetPixelFormat(pixelFormat);
        plane->SetHasAuxBuffer(hasAuxBuffer);
        plane->SetAuxPitch(auxPitch);
        plane->SetAuxOffset(auxOffset);
        plane->SetTilingFromModifier(modifier);

        if (hasAuxBuffer)
        {
            HWCLOGD_COND(eLogBuffer, "UpdateBufferPlane %s CRTC %d plane %d pixelFormat %d (Aux buffer - pitch %d offset %d modifier %llu)",
              buf->IdStr(strbuf), crtc ? crtc->GetCrtcId() : 0, plane ? plane->GetPlaneId() : 0, pixelFormat, auxPitch, auxOffset, modifier);
        }
        else
        {
            HWCLOGD_COND(eLogBuffer, "UpdateBufferPlane %s CRTC %d plane %d pixelFormat %d",
              buf->IdStr(strbuf), crtc ? crtc->GetCrtcId() : 0, plane ? plane->GetPlaneId() : 0, pixelFormat);
        }
    }
    else
    {
        HWCERROR(eCheckDrmFbId, "FB %d does not map to any open buffer", fbId);
    }
    HWCCHECK(eCheckDrmFbId);

    HWCCHECK(eCheckIvpProt);
    if (buf.get() && !buf->IsProtectionCorrect())
    {
        if (!buf->HasMediaDetailsEncrypted())
        {
            HWCERROR(eCheckIvpProt, "%s should be encrypted, but is not", buf->IdStr(strbuf));
        }
        else
        {
            HWCERROR(eCheckIvpProt, "%s should NOT be encrypted, but is", buf->IdStr(strbuf));
        }
    }

    plane->SetBuf(buf);

    return buf;
}

void DrmShimChecks::ValidateFrame(uint32_t crtcId, uint32_t nextFrame)
{
    HWCVAL_LOCK(_l,mMutex);
    mWorkQueue.Process();

    ssize_t ix = mCrtcs.indexOfKey(crtcId);
    if (ix >= 0)
    {
        DrmShimCrtc* crtc = mCrtcs.valueAt(ix);
        ValidateFrame(crtc, nextFrame, false);
    }
    else
    {
        HWCERROR(eCheckInvalidCrtc, "Unknown CRTC %d", crtcId);
    }
}

void DrmShimChecks::ValidateDrmReleaseTo(uint32_t connectorId)
{
    HWCVAL_LOCK(_l,mMutex);
    mWorkQueue.Process();

    ssize_t ix = mConnectors.indexOfKey(connectorId);
    if (ix >= 0)
    {
        const Connector& conn = mConnectors.valueAt(ix);
        DrmShimCrtc* crtc = conn.mCrtc;

        if (crtc)
        {
            HWCLOGD_COND(eLogParse, "ValidateDrmReleaseTo: connector %d crtc %p", connectorId, crtc);

            if (crtc->IsConnectedDisplay())
            {
                ValidateFrame(crtc, -1, true);
            }
        }
        else
        {
            HWCLOGD_COND(eLogParse, "ValidateDrmReleaseTo: NO crtc for connector %d", connectorId);
        }
    }
    else
    {
        HWCLOGD_COND(eLogParse, "ValidateDrmReleaseTo: Connector %d does not exist", connectorId);
    }
}



void DrmShimChecks::ValidateFrame(DrmShimCrtc* crtc, uint32_t nextFrame, bool drop)
{
    uint32_t disp = crtc->GetDisplayIx();
    if (disp == eNoDisplayIx)
    {
        // TODO use appropriate display index. Cached from before disconnect?
        HWCLOGD("CRTC %d disconnected from SF, skipping validation", crtc->GetCrtcId());
        return;
    }

    HWCLOGD_COND(eLogParse, "DrmShimChecks::ValidateFrame Validate crtc %d@%p displayIx %d nextFrame %d drop %d", crtc->GetCrtcId(), crtc, disp, nextFrame, drop);

    int currentFrame = mCurrentFrame[disp];
    mCurrentFrame[disp] = nextFrame;

    if (currentFrame > 0)
    {
        mLLQ[disp].LogQueue();
        HWCLOGD_COND(eLogParse, "DrmShimChecks::ValidateFrame Getting disp %d frame:%d from LLQ", disp, currentFrame);

        uint32_t srcDisp = crtc->GetSfSrcDisplayIx();

        // Don't generate "previous frame's fence was not signalled" errors if:
        // 1. We have some mosaic display stuff going on as the fence can't be signalled twice! OR
        // 2. we are suspending and hence are putting a blanking frame on the display.
        bool expectPrevFrameSignalled = (!crtc->IsMappedFromOtherDisplay()) && (nextFrame != 0);
        Hwcval::LayerList* ll = mLLQ[srcDisp].GetFrame(currentFrame, expectPrevFrameSignalled);

        if (crtc->DidSetDisplayFail())
        {
            // Don't validate if the SetDisplay failed as (a) we have already logged that error, (b) the Retire Fence will already have been signalled
            // and we don't want to generate an error to that effect.
            HWCLOGI("DrmShimChecks::ValidateFrame DidSetDisplayFail on CRTC %d failed, skip validation", crtc->GetCrtcId());
            return;
        }

        if (ll)
        {
            if (crtc->IsExternalDisplay())
            {
                SetExtendedModeExpectation(ll->GetVideoFlags().mSingleFullScreenVideo, true, currentFrame);
            }

            if (nextFrame > 0)
            {
                HWCLOGD_COND(eLogParse, "DrmShimChecks::ValidateFrame CRTC %d frame:%d", crtc->GetCrtcId(), currentFrame);
                crtc->Checks(ll, this, currentFrame);
            }
            else
            {
                // Display was turned off, so buffers may have been overwritten, so we can't validate.
            }
        }
        else
        {
            HWCLOGW("ValidateFrame CRTC %d NO FRAME %d", crtc->GetCrtcId(), currentFrame);
        }
    }

    crtc->PageFlipsSinceDPMS();
}

void DrmShimChecks::ValidateEsdRecovery(uint32_t d)
{
    HWCVAL_LOCK(_l,mMutex);
    mWorkQueue.Process();

    DrmShimCrtc* crtc = GetCrtcByDisplayIx(d);

    HWCLOGD_COND(eLogParse, "PARSED MATCHED {ESD%d}", d);
    if (crtc)
    {
        crtc->EsdStateTransition(HwcTestCrtc::eEsdAny, HwcTestCrtc::eEsdStarted);
    }
}

void DrmShimChecks::ValidateDisplayMapping(uint32_t connId, uint32_t crtcId)
{
    // Now we need to work out the display index based on (a) what is already connected and
    // (b) whether the connector is fixed or removable.

    // Is display index 0 already in use?
    // We can't do this from using mCrtcByDisplayIx because this may not be assigned yet.
    uint32_t crtcIdByDisplayIx[HWCVAL_MAX_CRTCS];
    memset(crtcIdByDisplayIx, 0, sizeof(crtcIdByDisplayIx));

    for (uint32_t i=0; i<mConnectors.size(); ++i)
    {
        Connector& conn = mConnectors.editValueAt(i);
        uint32_t id = mConnectors.keyAt(i);

        if (id == connId)
        {
            // We don't have to block this from reuse, it's the one we are reallocating
        }
        else if (conn.mDisplayIx != eNoDisplayIx)
        {
            if (conn.mCrtc)
            {
                crtcIdByDisplayIx[conn.mDisplayIx] = conn.mCrtc->GetCrtcId();
            }
            else
            {
                // Just say that the connector is in use
                crtcIdByDisplayIx[conn.mDisplayIx] = 0xffffffff;
            }
        }
    }

    uint32_t displayIx=0;
    if (crtcIdByDisplayIx[0])
    {
        if (crtcIdByDisplayIx[0] == crtcId)
        {
            HWCLOGD_COND(eLogHotPlug, "New Connection: Connector %d CRTC %d already associated with D0",
                connId, crtcId);
            return;
        }

        if (crtcIdByDisplayIx[1])
        {
            if (crtcIdByDisplayIx[1] == crtcId)
            {
                HWCLOGD_COND(eLogHotPlug, "New Connection: Connector %d CRTC %d already associated with D1",
                    connId, crtcId);
            }
            else
            {
                HWCLOGW("New Connection: Connector %d CRTC %d can't be used because D0 and D1 already associated",
                    connId, crtcId);
            }

            return;
        }

        // We can associate with display index 1
        displayIx = 1;
    }

    MapDisplay(displayIx, connId, crtcId);
}

void DrmShimChecks::ValidateDisplayUnmapping(uint32_t crtcId)
{
    ssize_t ix = mCrtcs.indexOfKey(crtcId);
    if (ix < 0)
    {
        HWCLOGW("Reset Connection: CRTC %d not found", crtcId);
        return;
    }

    DrmShimCrtc* crtc = mCrtcs.valueAt(ix);
    ALOG_ASSERT(crtc->GetCrtcId() == crtcId);

    uint32_t dix = crtc->GetDisplayIx();
    if (dix != eNoDisplayIx)
    {
        mCrtcByDisplayIx[crtc->GetDisplayIx()] = 0;
        crtc->SetDisplayIx(eNoDisplayIx);
    }

    for (uint32_t i=0; i<mConnectors.size(); ++i)
    {
        Connector& conn = mConnectors.editValueAt(i);
        if (conn.mCrtc == crtc)
        {
            conn.mDisplayIx = eNoDisplayIx;
            conn.mCrtc = 0;
        }
    }
}

// Display property query
// DO NOT CALL from locked code
uint32_t DrmShimChecks::GetDisplayProperty(uint32_t displayIx, HwcTestState::DisplayPropertyType prop)
{
    HWCVAL_LOCK(_l, mMutex);
    mWorkQueue.Process();

    DrmShimCrtc* crtc = GetCrtcByDisplayIx(displayIx);
    if (crtc == 0)
    {
        return 0;
    }

    switch (prop)
    {
        case HwcTestState::ePropConnectorId:
        {
            return crtc->GetConnector();
        }
        default:
        {
            // Test has requested an invalid property
            ALOG_ASSERT(0);
            return 0;
        }
    }
}



/// Move device-specific ids from old to new buffer
void DrmShimChecks::MoveDsIds(android::sp<DrmShimBuffer> existingBuf, android::sp<DrmShimBuffer> buf)
{
    DrmShimBuffer::FbIdVector& fbIds = existingBuf->GetFbIds();
    buf->GetFbIds() = fbIds;

    for (uint32_t i=0; i<fbIds.size(); ++i)
    {
        uint32_t fbId = fbIds.keyAt(i);
        mBuffersByFbId.replaceValueFor(fbId, buf);
    }
}

DrmShimCrtc* DrmShimChecks::GetCrtcByDisplayIx(uint32_t displayIx)
{
    return static_cast<DrmShimCrtc*>(GetHwcTestCrtcByDisplayIx(displayIx));
}

DrmShimCrtc* DrmShimChecks::GetCrtcByPipe(uint32_t pipe)
{
    return mCrtcByPipe[pipe];
}

void DrmShimChecks::MarkEsdRecoveryStart(uint32_t connectorId)
{
    ssize_t ix = mConnectors.indexOfKey(connectorId);
    if (ix >= 0)
    {
        DrmShimCrtc* crtc = mConnectors.valueAt(ix).mCrtc;

        if (crtc)
        {
            crtc->MarkEsdRecoveryStart();
        }
    }
}

/// Set reference to the DRM property manager
void DrmShimChecks::SetPropertyManager(Hwcval::PropertyManager& propMgr)
{
    mPropMgr = &propMgr;
    propMgr.SetTestKernel(this);
}

HwcTestKernel::ObjectClass DrmShimChecks::GetObjectClass(uint32_t objId)
{
    ssize_t ix = mPlanes.indexOfKey(objId);

    if (ix < 0)
    {
        ix = mCrtcs.indexOfKey(objId);

        if (ix < 0)
        {
            HWCLOGV_COND(eLogNuclear, "Object %d not found out of %d planes and %d crtcs", objId, mPlanes.size(), mCrtcs.size());
            return HwcTestKernel::eOther;
        }
        else
        {
            return HwcTestKernel::eCrtc;
        }
    }
    else
    {
        return HwcTestKernel::ePlane;
    }
}

DrmShimPlane* DrmShimChecks::GetDrmPlane(uint32_t drmPlaneId)
{
    DrmShimPlane* plane = mPlanes.valueFor(drmPlaneId);

    if (plane)
    {
        return plane;
    }

    // For nuclear spoofing only...
    if (mState->IsOptionEnabled(eOptSpoofNuclear))
    {
        // We are looking for a plane with the quoted Drm (SetDisplay) planeId.
        for (uint32_t i=0; i<mPlanes.size(); ++i)
        {
            plane = mPlanes.valueAt(i);

            if (plane->GetDrmPlaneId() == drmPlaneId)
            {
                return plane;
            }
        }
    }

    return 0;
}

bool DrmShimChecks::SimulateHotPlug(uint32_t displayTypes, bool connected)
{
    PushThreadState ts("DrmShimChecks::SimulateHotPlug");
    bool done = false;

    for (uint32_t i=0; i<HWCVAL_MAX_PIPES; ++i)
    {
        HwcTestCrtc* crtc = mCrtcByPipe[i];

        if (crtc)
        {
            if (crtc->IsHotPluggable())
            {
                // Allow filtering on the real display type
                // This makes sense only if we have eOptSpoofNoPanel set.
                // In this instance, the caller can for instance pass eFixed
                // for the displayTypes parameter and then only the panel would
                // be hot plugged/unplugged.
                if ((crtc->GetRealDisplayType() & displayTypes) != 0)
                {
                    done |= crtc->SimulateHotPlug(connected);
                }
            }
        }
    }

    return done;
}

bool DrmShimChecks::IsHotPluggableDisplayAvailable()
{
    if (!mState->GetNewDisplayConnectionState())
    {
        return false;
    }

    for (uint32_t i=0; i<HWCVAL_MAX_PIPES; ++i)
    {
        HwcTestCrtc* crtc = mCrtcByPipe[i];

        if (crtc)
        {
            if (crtc->IsHotPluggable())
            {
                return true;
            }
        }
    }

    return false;
}

void DrmShimChecks::CheckSetDDRFreq(uint64_t value)
{
    // 0 - normal
    // 1 - low freq
    HWCLOGD("DDR Frequency set to %s", value ? "LOW" : "NORMAL");
    mDDRMode = value;
}

bool DrmShimChecks::IsDDRFreqSupported()
{
    for (uint32_t i=0; i<mConnectors.size(); ++i)
    {
        const Connector& conn = mConnectors.valueAt(i);

        if (conn.mAttributes & eDDRFreq)
        {
            return true;
        }
    }

    return false;
}

bool DrmShimChecks::IsDRRSEnabled(uint32_t connId)
{
    ssize_t ix = mConnectors.indexOfKey(connId);

    if (ix >= 0)
    {
        const Connector& conn = mConnectors.valueAt(ix);

        return ((conn.mAttributes & eDRRS) != 0);
    }
    else
    {
        HWCLOGD("IsDRRSEnabled: connector %d not found", connId);
        return false;
    }
}

Hwcval::LogChecker* DrmShimChecks::GetParser()
{
    return &mDrmParser;
}

