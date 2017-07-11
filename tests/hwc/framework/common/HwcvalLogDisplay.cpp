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

File Name:      HwcvalLogDisplay.cpp

Description:    Hwc validation abstraction of a HWC logical display.

Environment:

Notes:

****************************************************************************/

#include "HwcTestState.h"
#include "HwcvalLogDisplay.h"

Hwcval::LogDisplayMapping::LogDisplayMapping()
  : mLogDisplayIx(eNoDisplayIx),
    mDisplayIx(eNoDisplayIx),
    mFlags(0),
    mSrcX(0),
    mSrcY(0),
    mSrcW(0),
    mSrcH(0),
    mDstX(0),
    mDstY(0),
    mDstW(0),
    mDstH(0)
{
}

Hwcval::LogDisplayMapping::LogDisplayMapping(uint32_t logDisp, uint32_t disp, uint32_t flags,
    uint32_t sx, uint32_t sy, uint32_t sw, uint32_t sh, uint32_t dx, uint32_t dy, uint32_t dw, uint32_t dh)
  : mLogDisplayIx(logDisp),
    mDisplayIx(disp),
    mFlags(flags),
    mSrcX(sx),
    mSrcY(sy),
    mSrcW(sw),
    mSrcH(sh),
    mDstX(dx),
    mDstY(dy),
    mDstW(dw),
    mDstH(dh)
{
}

void Hwcval::LogDisplayMapping::Log(const char* str)
{
    HWCLOGD_COND(eLogMosaic, "%s %d %d,%d %dx%d -> %d %d,%d %dx%d",
        str,
        mLogDisplayIx, mSrcX, mSrcY, mSrcW, mSrcH,
        mDisplayIx, mDstX, mDstY, mDstW, mDstH);
}

Hwcval::LogDisplay::LogDisplay(uint32_t displayIx)
  : mVSyncPeriod(0),
    mWidth(0),
    mHeight(0),
    mXDPI(0),
    mYDPI(0),
    mConfigId(0),
    mDisplayIx(displayIx)
{
}

void Hwcval::LogDisplay::SetConfigs(uint32_t* configs, size_t numConfigs)
{
    mConfigs.clear();
    mConfigs.appendArray(configs, numConfigs);

    if (numConfigs > 0)
    {
        for (uint32_t i=0; i<numConfigs; ++i)
        {
            if (configs[i] == mConfigId)
            {
                // Currently set config id is valid, so we can keep it
                HWCLOGD_COND(eLogHwcDisplayConfigs, "D%d: SetConfigs current config is still %x", mDisplayIx, mConfigId);
                return;
            }
        }

        mConfigId = configs[0];
        HWCLOGD_COND(eLogHwcDisplayConfigs, "D%d: SetConfigs current config is now %x", mDisplayIx, mConfigId);
    }
}

void Hwcval::LogDisplay::SetActiveConfig(uint32_t configId)
{
    if (configId != mConfigId)
    {
        HWCLOGD_COND(eLogHwcDisplayConfigs, "D%d: SetActiveConfig %x", mDisplayIx, configId);
        mConfigId = configId;
        mWidth = 0;
        mHeight = 0;
        mVSyncPeriod = 0;
    }
}

void Hwcval::LogDisplay::SetDisplayAttributes(uint32_t configId, const uint32_t* attributes, int32_t* values)
{
    if (configId == mConfigId)
    {
        HWCLOGD_COND(eLogHwcDisplayConfigs, "D%d: SetDisplayAttributes, config %x is current", mDisplayIx, configId);
        for (uint32_t i=0; attributes[i]; ++i)
        {
            switch(attributes[i])
            {
                case HWC_DISPLAY_VSYNC_PERIOD :
                    mVSyncPeriod = values[i];
                    break;

                case HWC_DISPLAY_WIDTH        :
                    mWidth = values[i];
                    HWCLOGD_COND(eLogHwcDisplayConfigs, "D%d LogDisplay: set width to %d", mDisplayIx, mWidth);
                    break;

                case HWC_DISPLAY_HEIGHT       :
                    mHeight = values[i];
                    HWCLOGD_COND(eLogHwcDisplayConfigs, "D%d LogDisplay: set height to %d", mDisplayIx, mHeight);
                    break;

                case HWC_DISPLAY_DPI_X        :
                    mXDPI = values[i];
                    break;

                case HWC_DISPLAY_DPI_Y        :
                    mYDPI = values[i];
                    break;

                default:
                    HWCLOGW("Unknown display attribute %d", attributes[i]);
            };
        }
    }
    else
    {
        HWCLOGD("D%d: LogDisplay::SetDisplayAttributes: config %d is not current config %x", mDisplayIx, configId, mConfigId);
    }
}
